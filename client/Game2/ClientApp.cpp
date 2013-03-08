
// ClientApp.cpp
// Experimental game shell.
//
// Copyright (c) 2010, 2013 Michael Imamura.
//
// Licensed under GrokkSoft HoverRace SourceCode License v1.0(the "License");
// you may not use this file except in compliance with the License.
//
// A copy of the license should have been attached to the package from which
// you have taken this file. If you can not find the license you can not use
// this file.
//
//
// The author makes no representations about the suitability of
// this software for any purpose.  It is provided "as is" "AS IS",
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
//
// See the License for the specific language governing permissions
// and limitations under the License.

#include "StdAfx.h"

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#ifdef WITH_SDL_PANGO
#	include <SDL_Pango.h>
#endif

#include "../../engine/Exception.h"
#include "../../engine/Display/Label.h"
#include "../../engine/Display/SDL/SdlDisplay.h"
#include "../../engine/MainCharacter/MainCharacter.h"
#include "../../engine/Model/Track.h"
#include "../../engine/Parcel/TrackBundle.h"
#include "../../engine/Util/Config.h"
#include "../../engine/Util/FuzzyLogic.h"
#include "../../engine/Util/DllObjectFactory.h"
#include "../../engine/Util/Str.h"
#include "../../engine/Util/WorldCoordinates.h"
#include "../../engine/VideoServices/SoundServer.h"
#include "../../engine/VideoServices/VideoBuffer.h"

#include "Control/Controller.h"
#include "Control/UiHandler.h"
#include "HoverScript/ClientScriptCore.h"
#include "HoverScript/GamePeer.h"
#include "HoverScript/HighConsole.h"
#include "HoverScript/SessionPeer.h"
#include "HoverScript/SysEnv.h"
#include "ClientSession.h"
#include "GameScene.h"
#include "HighObserver.h"
#include "MessageScene.h"
#include "PaletteScene.h"
#include "Rulebook.h"
#include "Scene.h"

#ifdef _WIN32
#	include "resource.h"
#endif

#include "ClientApp.h"

using namespace HoverRace::Client::HoverScript;
using namespace HoverRace::Util;
namespace SoundServer = HoverRace::VideoServices::SoundServer;
using HoverRace::Client::Control::InputEventController;

namespace HoverRace {
namespace Client {

namespace {
	// SDL user event codes.
	enum {
		REQ_EVT_SCENE_PUSH,
		REQ_EVT_SCENE_POP,
		REQ_EVT_SCENE_REPLACE,
	};

	/**
	 * Used by @c REQ_EVT_SCENE_PUSH and @c REQ_SCENE_REPLACE to transfer
	 * the requested scene.
	 */ 
	struct SceneHolder {
		SceneHolder(const ScenePtr &scene) : scene(scene) { }
		ScenePtr scene;
	};
}

class ClientApp::UiInput : public Control::UiHandler
{
	virtual void OnConsole()
	{
		throw UnimplementedExn("ClientApp::UiInput::OnConsole()");
	}
};

ClientApp::ClientApp() :
	SUPER(),
	uiInput(std::make_shared<UiInput>()),
	sceneStack(), fgScene(sceneStack.rend()),
	fpsLbl(), frameCount(0), lastTimestamp(0), fps(0.0)
{
	Config *cfg = Config::GetInstance();

	// Engine initialization.
	MR_InitTrigoTables();
	MR_InitFuzzyModule();
	SoundServer::Init();
	DllObjectFactory::Init();
	MainCharacter::MainCharacter::RegisterFactory();

	if (SDL_Init(SDL_INIT_VIDEO) == -1)
		throw Exception("SDL initialization failed");
#	ifdef WITH_SDL_PANGO
		SDLPango_Init();
#	endif

	// Create the system console and execute the init script.
	// This allows the script to modify the configuration (e.g. for unit tests).
	scripting = (new ClientScriptCore())->Reset();
	gamePeer = new GamePeer(scripting, this);
	sysEnv = new SysEnv(scripting, gamePeer);
	OS::path_t &initScript = cfg->runtime.initScript;
	if (!initScript.empty()) {
		sysEnv->RunScript(initScript);
	}

	// With SDL we can only get the desktop resolution before the first call to
	// SDL_SetVideoMode().
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();

	// Create the main window and SDL surface.
	//TODO: Select which display to use.
	display = new Display::SDL::SdlDisplay();
	display->OnDesktopModeChanged(videoInfo->current_w, videoInfo->current_h);

	// Set window position and icon (platform-dependent).
	SDL_SysWMinfo wm;
	SDL_VERSION(&wm.version);
	if (SDL_GetWMInfo(&wm) != 0) {
#		ifdef _WIN32
			HWND hwnd = mainWnd = wm.window;
			SetWindowPos(hwnd, HWND_TOP,
				cfg->video.xPos, cfg->video.yPos, 0, 0,
				SWP_NOSIZE);

			// Set icon.
			// On Windows, the icon is embedded as a resource.
			HMODULE hmod = GetModuleHandleW(NULL);
			LPWSTR iconRes = MAKEINTRESOURCEW(IDI_HOVER_ICON);
			HANDLE ico;
			ico = LoadImageW(hmod, iconRes, IMAGE_ICON,
				GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
			if (ico != NULL)
				SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(ico));
			ico = LoadImageW(hmod, iconRes, IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			if (ico != NULL)
				SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(ico));
#		else
			mainWnd = wm.info.x11.window;
			// On non-Win32 we prefer to let the window manager decide the
			// position of the window.
			/*TODO
			SDL_WM_SetIcon(SDL_LoadBMP(cfg->GetMediaPath("icon.bmp").file_string().c_str()), 0);
			*/
#		endif
	}

	controller = new InputEventController(mainWnd, uiInput);

	RefreshTitleBar();

	fpsLbl = new Display::Label("FPS:",
		Display::UiFont(cfg->GetDefaultFontName(), 20, Display::UiFont::BOLD),
		Display::Color(0xff, 0xff, 0x7f, 0x00));
	fpsLbl->AttachView(*display);
}

ClientApp::~ClientApp()
{
	delete fpsLbl;
	delete sysEnv;
	delete gamePeer;
	delete scripting;
	delete display;
	delete controller;

	// Engine shutdown.
	DllObjectFactory::Clean(FALSE);
	SoundServer::Close();

	SDL_Quit();
}

void ClientApp::RefreshTitleBar()
{
	Config *cfg = Config::GetInstance();

	std::ostringstream oss;
	oss << PACKAGE_NAME " " << cfg->GetVersion();
	if (cfg->IsPrerelease()) {
		oss << " (" << pgettext("Version", "testing") << ')';
	}
	if (cfg->runtime.silent) {
		oss << " (" << _("silent mode") << ')';
	}

	SDL_WM_SetCaption(oss.str().c_str(), NULL);
}

/**
 * Handle when the window is resized.
 * @param w The new width.
 * @param h The new height.
 */
void ClientApp::OnWindowResize(int w, int h)
{
	Config::cfg_video_t &vidCfg = Config::GetInstance()->video;
	vidCfg.xRes = w;
	vidCfg.yRes = h;

	display->OnDisplayConfigChanged();
}

/**
 * Increment the frame counter for stats purposes.
 * This should be called once per frame.
 */
void ClientApp::IncFrameCount()
{
	static boost::format fpsFmt("FPS: %0.2f\n");

	// Don't start counting until the first frame.
	if (lastTimestamp == 0) lastTimestamp = OS::Time();

	++frameCount;
	OS::timestamp_t curTimestamp = OS::Time();
	OS::timestamp_t diff = OS::TimeDiff(curTimestamp, lastTimestamp);

	if (diff > 1000) {
		fps = ((double)frameCount) / (diff / 1000.0);
		lastTimestamp = curTimestamp;
		frameCount = 0;

		if (Config::GetInstance()->runtime.showFramerate) {
			fpsLbl->SetText(boost::str(fpsFmt % fps));
		}
		/*
		OutputDebugString(boost::str(fpsFmt % fps).c_str());
		*/
	}
}

void ClientApp::AdvanceScenes(Util::OS::timestamp_t tick)
{
	for (auto iter = sceneStack.begin(); iter != sceneStack.end(); ++iter) {
		(*iter)->Advance(tick);
		// If the scene has entered the STOPPED state, it's deletable.
		if ((*iter)->GetPhase() == Scene::Phase::STOPPED) {
			iter = sceneStack.erase(iter);
		}
	}
}

void ClientApp::RenderScenes()
{
	bool showFps = Config::GetInstance()->runtime.showFramerate;

	IncFrameCount();

	if (sceneStack.empty()) {
		VideoServices::VideoBuffer::Lock lock(display->GetLegacyDisplay());
		display->GetLegacyDisplay().Clear();
	}
	else {
		BOOST_FOREACH(const ScenePtr &scene, sceneStack) {
			scene->PrepareRender();
		}
		if (showFps) fpsLbl->PrepareRender();

		BOOST_FOREACH(const ScenePtr &scene, sceneStack) {
			scene->Render();
		}
		if (showFps) fpsLbl->Render();
	}
	
	display->Flip();
}

void ClientApp::MainLoop()
{
	bool quit = false;
	SDL_Event evt;

	// Fire all on_init handlers and check if a new session was requested.
	gamePeer->OnInit();
	RulebookPtr rules = gamePeer->RequestedNewSession();
	if (rules != NULL) {
		NewLocalSession(rules);
	}
	else {
		ReplaceScene(std::make_shared<PaletteScene>(*display));
	}

	Config::cfg_runtime_t &runtimeCfg = Config::GetInstance()->runtime;
	if (!runtimeCfg.skipStartupWarning && runtimeCfg.initScript.empty()) {
		PushScene(std::make_shared<MessageScene>(*display, *this,
			"Warning",
			"This is the unstable \"2.0\" branch of HoverRace.\n"
			"For the current stable branch, switch to the \"1.24\" branch.\n"
			"To skip this warning, use --skip-startup-warning or specify a "
			"startup script with --exec."));
	}

#	ifdef WITH_SDL_OIS_INPUT
		std::vector<SDL_Event> deferredEvents;
#	endif

	while (!quit) {
		OS::timestamp_t tick = OS::Time();

		while (SDL_PollEvent(&evt) && !quit) {
			switch (evt.type) {
				case SDL_QUIT:
					quit = true;
					break;

				case SDL_VIDEORESIZE:
					OnWindowResize(evt.resize.w, evt.resize.h);
					break;

				// Certain SDL events need to be processed by the SDL-OIS
				// bridge, so we need to save them for later.
#				ifdef WITH_SDL_OIS_INPUT
					case SDL_KEYDOWN:
					case SDL_KEYUP:
						deferredEvents.push_back(evt);
						break;
#				endif

				case SDL_USEREVENT:
					switch (evt.user.code) {
						case REQ_EVT_SCENE_PUSH: {
							SceneHolder *holder = static_cast<SceneHolder*>(evt.user.data1);
							PushScene(holder->scene);
							delete holder;
							break;
						}

						case REQ_EVT_SCENE_POP:
							PopScene();
							break;

						case REQ_EVT_SCENE_REPLACE: {
							SceneHolder *holder = static_cast<SceneHolder*>(evt.user.data1);
							ReplaceScene(holder->scene);
							delete holder;
							break;
						}
					}
					break;
			}
		}
		if (quit) break;

#		ifdef WITH_SDL_OIS_INPUT
			BOOST_FOREACH(SDL_Event &evt, deferredEvents) {
				SDL_PushEvent(&evt);
			}
			deferredEvents.clear();
#		endif
		controller->Poll();

		AdvanceScenes(tick);
		RenderScenes();
	}

	TerminateAllScenes();
}

void ClientApp::NewLocalSession(RulebookPtr rules)
{
	//TODO: Confirm ending the current session.

	//TODO: Prompt the user for a track name.
	try {
		ReplaceScene(std::make_shared<GameScene>(this, *display, scripting, gamePeer, rules));
	}
	catch (Parcel::ObjStreamExn&) {
		throw;
	}
}

/**
 * Remove the foreground scene.
 * @note This must only be called from the main thread.
 * This indicates that there is no scene capable of receiving input.
 */
void ClientApp::SetForegroundScene()
{
	fgScene = sceneStack.rend();
	//TODO: Load failsafe controller mapping.
	controller->ClearActionMap();

	// Enable the cursor to make it easier for users to click the "Close"
	// button on the main window.
	SDL_ShowCursor(SDL_ENABLE);
}

/**
 * Switch which scene has input focus.
 * @note This must only be called from the main thread.
 * @param iter An iterator pointing to the new foreground scene.
 */
void ClientApp::SetForegroundScene(const sceneStack_t::reverse_iterator &iter)
{
	if (iter == sceneStack.rend()) {
		SetForegroundScene();
	}
	else {
		fgScene = iter;

		// Load controller mapping from new foreground scene.
		controller->ClearActionMap();
		Scene *scene = fgScene->get();
		scene->SetupController(*controller);
		SDL_ShowCursor(scene->IsMouseCursorEnabled() ? SDL_ENABLE : SDL_DISABLE);
	}
}

/**
 * Push a new scene to the foreground.
 * @note This must only be called from the main thread.
 * @param scene The scene to push.
 */
void ClientApp::PushScene(const ScenePtr &scene)
{
	sceneStack.push_back(scene);
	SetForegroundScene(sceneStack.rbegin());
	scene->SetPhase(Scene::Phase::STARTING);
}

/**
 * Return to the previous scene, if any.
 * @note This must only be called from the main thread.
 */
void ClientApp::PopScene()
{
	if (fgScene != sceneStack.rend()) {
		(*fgScene)->SetPhase(Scene::Phase::STOPPING);

		// Find the first non-stopping scene.
		bool found = false;
		for (auto iter = sceneStack.rbegin(); iter != sceneStack.rend(); ++iter) {
			Scene::Phase::phase_t phase = (*iter)->GetPhase();
			if (phase != Scene::Phase::STOPPING && phase != Scene::Phase::STOPPED) {
				SetForegroundScene(iter);
				found = true;
				break;
			}
		}
		if (!found) SetForegroundScene();
	}
}

/**
 * Replace the current scene and all background scenes with a new
 * foreground scene.
 * @note This must only be called from the main thread.
 * @param scene The scene to push.
 */
void ClientApp::ReplaceScene(const ScenePtr &scene)
{
	BOOST_FOREACH(ScenePtr &s, sceneStack) {
		s->SetPhase(Scene::Phase::STOPPING);
	}
	PushScene(scene);
}

/**
 * Immediately remove all scenes.
 * @note This must only be called from the main thread.
 */
void ClientApp::TerminateAllScenes()
{
	sceneStack.clear();
	SetForegroundScene();
}

void ClientApp::RequestPushScene(const ScenePtr &scene)
{
	SDL_Event evt;
	evt.type = SDL_USEREVENT;
	evt.user.code = REQ_EVT_SCENE_PUSH;
	evt.user.data1 = new SceneHolder(scene);
	SDL_PushEvent(&evt);
}

void ClientApp::RequestPopScene()
{
	SDL_Event evt;
	evt.type = SDL_USEREVENT;
	evt.user.code = REQ_EVT_SCENE_POP;
	SDL_PushEvent(&evt);
}

void ClientApp::RequestReplaceScene(const ScenePtr &scene)
{
	SDL_Event evt;
	evt.type = SDL_USEREVENT;
	evt.user.code = REQ_EVT_SCENE_REPLACE;
	evt.user.data1 = new SceneHolder(scene);
	SDL_PushEvent(&evt);
}

void ClientApp::RequestShutdown()
{
	SDL_Event evt;
	evt.type = SDL_QUIT;
	SDL_PushEvent(&evt);
}

void ClientApp::SignalServerHasChanged()
{
	throw UnimplementedExn("ClientApp::SignalServerHasChanged()");
}

void ClientApp::ChangeAutoUpdates(bool newSetting)
{
	throw UnimplementedExn("ClientApp::ChangeAutoUpdates(bool)");
}

void ClientApp::AssignPalette()
{
	throw UnimplementedExn("ClientApp::AssignPalette()");
}

VideoServices::VideoBuffer *ClientApp::GetVideoBuffer() const
{
	return &display->GetLegacyDisplay();
}

Control::InputEventController *ClientApp::ReloadController()
{
	//TODO: Notify current scene that controller is changing.
	delete controller;
	return (controller = new InputEventController(mainWnd, uiInput));
}

}  // namespace HoverScript
}  // namespace Client


// LoadingScene.h
//
// Copyright (c) 2014, 2015 Michael Imamura.
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

#pragma once

#include "GameDirector.h"

#include "FormScene.h"

namespace HoverRace {
	namespace Display {
		class ScreenFade;
	}
	namespace Util {
		class Loader;
	}
}

namespace HoverRace {
namespace Client {

/**
 * Base class for "Please Wait" loading scenes.
 *
 * These scenes provide a way to wait for the previous scene stack to finish
 * unloading resources then allow the new scene to load.
 *
 * To use this scene, enqueue tasks to the loader, then push the scene that
 * depends on the loader, then push this scene.  When all of the resources
 * have been loaded, this scene will automatically pop itself.
 *
 * @author Michael Imamura
 */
class LoadingScene : public FormScene
{
	using SUPER = FormScene;

public:
	LoadingScene(Display::Display &display, GameDirector &director,
		const std::string &name = "Loading");
	virtual ~LoadingScene();

public:
	std::shared_ptr<Util::Loader> ShareLoader() const { return loader; }

public:
	bool IsMouseCursorEnabled() const override { return false; }

protected:
	void OnPhaseChanged(Phase oldPhase) override;

public:
	void OnPhaseTransition(double progress) override;
	void PrepareRender() override;
	void Render() override;

private:
	GameDirector &director;
	bool loading;
	std::shared_ptr<Util::Loader> loader;
	std::unique_ptr<Display::ScreenFade> fader;
};

}  // namespace Client
}  // namespace HoverRace

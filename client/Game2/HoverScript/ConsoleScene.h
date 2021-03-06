
// ConsoleScene.h
//
// Copyright (c) 2013, 2015-2016 Michael Imamura.
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

#include "../../../engine/Control/Action.h"
#include "../UiScene.h"
#include "SysConsole.h"

namespace HoverRace {
	namespace Client {
		class GameDirector;
	}
	namespace Display {
		class ActiveText;
		class Display;
		class Label;
		class ScreenFade;
	}
}

namespace HoverRace {
namespace Client {
namespace HoverScript {

/**
 * UI for the system console.
 * @author Michael Imamura
 */
class ConsoleScene : public UiScene
{
	using SUPER = UiScene;

public:
	ConsoleScene(Display::Display &display, GameDirector &director,
		SysConsole &console);
	virtual ~ConsoleScene();

private:
	void OnDisplayConfigChanged();

	void OnConsoleToggle();
	void OnConsoleUp();
	void OnConsoleDown();
	void OnConsoleTop();
	void OnConsoleBottom();
	void OnConsolePrevCmd();
	void OnConsoleNextCmd();
	void OnTextInput(const std::string &s);
	void OnTextControl(Control::TextControl::key_t key);

	void OnLogCleared();
	void OnLogAdded(int idx);

	void AppendLogLine(const SysConsole::LogLine &line);
	void UpdateCommandLine();
	void Layout() override;

public:
	// Scene
	void AttachController(Control::InputEventController &controller,
		ConnList &conns) override;
	void DetachController(Control::InputEventController &controller,
		ConnList &conns) override;
public:
	void Advance(Util::OS::timestamp_t tick) override;
	void PrepareRender() override;
	void Render() override;

private:
	GameDirector &director;
	SysConsole &console;

	class LogLines;
	std::unique_ptr<LogLines> logLines;
	int lastLogIdx;
	bool logsChanged;

	boost::signals2::scoped_connection displayConfigChangedConn;

	boost::signals2::scoped_connection logClearedConn;
	boost::signals2::scoped_connection logAddedConn;

	bool layoutChanged;

	std::unique_ptr<Display::ScreenFade> fader;
	std::unique_ptr<Display::ActiveText> inputLbl;

	bool cursorOn;
	Util::OS::timestamp_t cursorTick;

	std::unique_ptr<Display::Label> measureLbl;  ///< Used to measure the size of glyphs.
	Vec2 charSize;
};

}  // namespace HoverScript
}  // namespace Client
}  // namespace HoverRace

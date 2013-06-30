
// FormScene.h
//
// Copyright (c) 2013 Michael Imamura.
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

#include "UiScene.h"

namespace HoverRace {
	namespace Display {
		class Display;
	}
}

namespace HoverRace {
namespace Client {

/**
 * Base class for full-scene menus and the like.
 * @author Michael Imamura
 */
class FormScene : public UiScene
{
	typedef UiScene SUPER;
	public:
		FormScene(Display::Display &display, const std::string &name="");
		virtual ~FormScene();

	public:
		virtual void AttachController(Control::InputEventController &controller);
		virtual void DetachController(Control::InputEventController &controller);

	private:
		void OnMouseMoved(const Display::Vec2 &pos);
		void OnMousePressed(const Control::Mouse::Click &click);
		void OnMouseReleased(const Control::Mouse::Click &click);

	protected:
		Display::Display &display;
	private:
		boost::signals2::connection mouseMovedConn;
		boost::signals2::connection mousePressedConn;
		boost::signals2::connection mouseReleasedConn;
};

}  // namespace Client
}  // namespace HoverRace
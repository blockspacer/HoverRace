
// FormScene.h
//
// Copyright (c) 2013, 2015 Michael Imamura.
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

#include "../../engine/Vec.h"

#include "UiScene.h"

namespace HoverRace {
	namespace Control {
		namespace Mouse {
			struct Click;
			struct Scroll;
		}
	}
	namespace Display {
		class Container;
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
	using SUPER = UiScene;

public:
	FormScene(Display::Display &display, const std::string &name = "");
	virtual ~FormScene();

public:
	void AttachController(Control::InputEventController &controller,
		ConnList &conns) override;

private:
	void OnMouseMoved(const Vec2 &pos);
	bool OnMousePressed(const Control::Mouse::Click &click);
	bool OnMouseReleased(const Control::Mouse::Click &click);
	bool OnMouseScrolled(const Control::Mouse::Scroll &scroll);

protected:
	Display::Container *GetRoot() const { return root.get(); }

public:
	void PrepareRender() override;
	void Render() override;

protected:
	Display::Display &display;
private:
	std::shared_ptr<Display::Container> root;
};

}  // namespace Client
}  // namespace HoverRace

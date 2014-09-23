
// BulletinBoard.h
//
// Copyright (c) 2014 Michael Imamura.
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

#include "../../engine/Display/Container.h"

namespace HoverRace {
	namespace Client {
		class Announcement;
	}
	namespace Display {
		class Display;
	}
}

namespace HoverRace {
namespace Client {

/**
 * Container for active announcements.
 * @author Michael Imamura
 */
class BulletinBoard : public Display::Container
{
	typedef Display::Container SUPER;
public:
	BulletinBoard(Display::Display &display);
	virtual ~BulletinBoard();

public:
	void Announce(std::shared_ptr<Announcement> ann);

protected:
	void Layout() override;

private:
	class Bulletin;
	typedef std::tuple<
		std::shared_ptr<Announcement>,
		std::shared_ptr<Bulletin>> bulletin_t;
	std::list<bulletin_t> bulletins;
};

}  // namespace HoverScript
}  // namespace Client

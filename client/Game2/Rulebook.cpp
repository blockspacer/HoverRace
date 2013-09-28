
// Rulebook.cpp
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

#include "StdAfx.h"

#include "../../engine/Script/Core.h"

#include "HoverScript/PlayerPeer.h"
#include "HoverScript/SessionPeer.h"
#include "ClientSession.h"

#include "Rulebook.h"

namespace HoverRace {
namespace Client {

namespace {
	class ConstantRule : public Rule
	{
		typedef Rule SUPER;
		public:
			ConstantRule(const luabind::object &val) : SUPER(), val(val) { }
			virtual ~ConstantRule() { }

		public:
			virtual luabind::object GetDefault() const { return val; }

		private:
			luabind::object val;
	};
}

Rulebook::Rulebook(Script::Core *scripting, const std::string &name,
                   const std::string &title,
                   const std::string &description,
                   int maxPlayers) :
	scripting(scripting), name(name), title(title), description(description),
	maxPlayers(maxPlayers),
	rules(),
	onPreGame(scripting), onPostGame(scripting), onPlayerInit(scripting)
{
}

void Rulebook::AddRule(const std::string &name, const luabind::object &obj)
{
	rules.insert(rules_t::value_type(name, std::make_shared<ConstantRule>(obj)));
}

luabind::object Rulebook::CreateDefaultRules() const
{
	luabind::object obj = luabind::newtable(scripting->GetState());

	BOOST_FOREACH(auto ent, rules) {
		obj[ent.first] = ent.second->GetDefault();
	}

	return obj;
}

void Rulebook::SetOnPreGame(const luabind::object &fn)
{
	onPreGame.AddHandler(fn);
}

void Rulebook::OnPreGame(HoverScript::SessionPeerPtr session) const
{
	onPreGame.CallHandlers(luabind::object(scripting->GetState(), session));
}

void Rulebook::SetOnPostGame(const luabind::object &fn)
{
	onPostGame.AddHandler(fn);
}

void Rulebook::OnPostGame(HoverScript::SessionPeerPtr session) const
{
	onPostGame.CallHandlers(luabind::object(scripting->GetState(), session));
}

void Rulebook::SetOnPlayerInit(const luabind::object &fn)
{
	onPlayerInit.AddHandler(fn);
}

void Rulebook::OnPlayerInit(HoverScript::SessionPeerPtr session,
                            std::shared_ptr<HoverScript::PlayerPeer> player) const
{
	lua_State *L = scripting->GetState();
	onPlayerInit.CallHandlers(luabind::object(L, session),
		luabind::object(L, player));
}

}  // namespace Client
}  // namespace HoverRace


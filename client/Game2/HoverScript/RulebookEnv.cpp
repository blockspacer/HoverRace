
// RulebookEnv.cpp
//
// Copyright (c) 2013-2014, 2016 Michael Imamura.
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

#include <boost/filesystem.hpp>

#include <luabind/class.hpp>
#include <luabind/detail/class_rep.hpp>

#include <utf8/utf8.h>

#include "../../../engine/Script/Core.h"
#include "../../../engine/Util/Config.h"
#include "../../../engine/Util/Log.h"
#include "../../../engine/Util/OS.h"
#include "../../../engine/Util/Str.h"

#include "../Rulebook.h"

#include "RulebookEnv.h"

using namespace HoverRace::Script;
using namespace HoverRace::Util;
namespace fs = boost::filesystem;

namespace HoverRace {
namespace Client {
namespace HoverScript {

namespace {

/**
 * Convert a Lua index into an absolute index.
 * @param L The current Lua state.
 * @param i The index to convert (may be relative or absolute).
 * @return The absolute index.
 */
int LuaAbsIndex(lua_State *L, int i)
{
	// Same logic as lauxlib's abs_index macro.
	return (i > 0 || i <= LUA_REGISTRYINDEX ? i : lua_gettop(L) + i + 1);
}

/**
 * Copy the contents of one Lua table into another.
 * The stack is unchanged upon return.
 * @param destIdx The stack index of the destination table.
 * @param srcIdx The stack index of the source table.
 */
void LuaMergeTables(lua_State *L, int destIdx, int srcIdx)
{
	destIdx = LuaAbsIndex(L, destIdx);
	srcIdx = LuaAbsIndex(L, srcIdx);

	// Initial stack: (empty)

	lua_pushnil(L);  // nil

	while (lua_next(L, srcIdx)) {
		// key value
		lua_pushvalue(L, -2);  // key value key
		lua_insert(L, -2); // key key value
		lua_settable(L, destIdx); // key
	}
	// (empty)
}

const luabind::object ExpectHandler(Script::Core &scripting,
	const luabind::object &props, const char *name)
{
	using namespace luabind;

	const object obj = props[name];

	int objType = type(obj);
	if (objType != LUA_TNIL && objType != LUA_TFUNCTION) {
		luaL_error(scripting.GetState(),
			"'%s' is required to be a function or nil", name);
		return *scripting.NIL;
	}

	return obj;
}

/**
 * Generate a unique name for a Lua subclass for class registration.
 *
 * This name is only for debugging purposes; subclasses generated via
 * the "Player", "Session", etc. functions are returned as values, so the
 * script can choose whatever "name" it wants, or leave it unnamed
 * altogether (which is the common case).
 *
 * @return The new unique global name.
 */
std::string AutoName()
{
	static int idx = 0;
	std::ostringstream oss;
	oss << "[RulebookEnv]#autoclass_" << idx++;
	return oss.str();
}

/**
 * Check if the module path passed to @c require() is allowed.
 * @param s The module path.
 * @return @c true if valid, @c false if not.
 */
bool IsValidModulePath(const std::string &s)
{
	MR_UInt32 ch;
	for (auto iter = s.cbegin(), iend = s.cend(); iter != iend;) {
		ch = utf8::next(iter, iend);

		if (!(ch == '-' ||
			(ch >= '0' && ch <= '9') ||
			(ch >= 'A' && ch <= 'Z') ||
			ch == '_' ||
			(ch >= 'a' && ch <= 'z')))
		{
			return false;
		}
	}
	return true;
}

}  // namespace

/**
 * Constructor.
 *
 * The root directory of the rulebook is where the rulebook scripts will be
 * found; a rulebook is not allowed to access any scripts outside of its
 * root directory.
 *
 * @param scripting The scripting context.
 * @param basePath The root directory of the rulebook.
 * @param rulebook The rulebook bound to this environment.
 */
RulebookEnv::RulebookEnv(Script::Core &scripting,
	const Util::OS::path_t &basePath, Rulebook &rulebook) :
	SUPER(scripting),
	basePath(basePath),
	rulebook(rulebook),
	requireCache(scripting)
{
	lua_newtable(scripting.GetState());
	requireCache.SetFromStack();
}

RulebookEnv::~RulebookEnv()
{
}

void RulebookEnv::InitEnv()
{
	using namespace luabind;

	SUPER::InitEnv();

	// Start with the standard global environment.
	CopyGlobals();

	// Register our MetaPlayer subclasser.
	InitCFn("Player", RulebookEnv::LPlayer);

	// Register our MetaSession subclasser.
	InitCFn("Session", RulebookEnv::LSession);

	// Register our custom rulebook-aware "require()".
	InitCFn("require", RulebookEnv::LRequire);

	// Register our Rulebook factory.
	InitCFn("Rulebook", RulebookEnv::LRulebookStage1);
}

/**
 * Register a member function in a table.
 *
 * This is expected to be called from InitEnv() to add global functions to
 * the environment.
 *
 * The top of the Lua stack must be the destination table.
 *
 * The function being registered will receive a single upvalue, which is the
 * @c this pointer.
 *
 * @param name The Lua name of the function.
 * @param fn The function to register.
 */
void RulebookEnv::InitCFn(const char *name, lua_CFunction fn)
{
	lua_State *L = GetState();

	// Initial stack: table

	lua_pushlightuserdata(L, this);  // table this
	lua_pushcclosure(L, fn, 1);  // table fn
	lua_pushstring(L, name);  // table fn str
	lua_insert(L, -2);  // table str fn
	lua_rawset(L, -3);  // table
}

void RulebookEnv::DefineRulebook(const std::string &name,
	const luabind::object &defn)
{
	using namespace luabind;

	Script::Core &scripting = GetScripting();
	lua_State *L = scripting.GetState();

	if (type(defn) != LUA_TTABLE) {
		luaL_error(L, "Expected table.");
		return;
	}

	const object &titleObj = defn["title"];
	if (type(titleObj) != LUA_TSTRING) {
		luaL_error(L, "'title' is required to be a string.");
		return;
	}
	const std::string title = object_cast<std::string>(titleObj);

	const object &descObj = defn["description"];
	std::string desc;
	switch (type(descObj)) {
		case LUA_TNIL:
			break;
		case LUA_TSTRING:
			desc = object_cast<std::string>(descObj);
			break;
		default:
			luaL_error(L, "Expected 'description' to be a string.");
			return;
	}

	const object &maxPlayersObj = defn["max_players"];
	int maxPlayers = 4;
	switch (type(maxPlayersObj)) {
		case LUA_TNIL:
			break;
		case LUA_TNUMBER:
			maxPlayers = object_cast<int>(maxPlayersObj);
			break;
		default:
			luaL_error(L, "Expected 'max_players' to be a number.");
			return;
	}

	rulebook.SetMetadata(name, title, desc, maxPlayers);

	const object &rulesObj = defn["rules"];
	switch (type(rulesObj)) {
		case LUA_TNIL:
			break;
		case LUA_TTABLE:
			DefineRules(rulesObj);
			break;
		default:
			luaL_error(L, "Expected 'rules' to be a table.");
			return;
	}

	rulebook.SetOnLoad(ExpectHandler(scripting, defn, "on_load"));

	HR_LOG(info) << "Registered: " << name << ": " << title << ", " << desc;
}

void RulebookEnv::DefineRules(const luabind::object &rulesObj)
{
	using namespace luabind;

	for (iterator iter(rulesObj), end; iter != end; ++iter) {
		const std::string name = object_cast<std::string>(iter.key());
		const object ruleObj = *iter;

		//TODO: Check type of ruleObj and cast to Rule if necessary.
		rulebook.AddRule(name, ruleObj);

		HR_LOG(info) << "Added rule '" << name << "' with type " <<
			type(ruleObj) << ".";
	}
}

/**
 * Run the boot script for a rulebook.
 */
bool RulebookEnv::RunRulebookScript()
{
	auto bootPath = basePath;
	bootPath /= Str::UP("rulebook.lua");

	if (!fs::exists(bootPath)) {
		HR_LOG(info) << "Rulebook path does not have a rulebook.lua: " <<
			basePath;
		return false;
	}

	HR_LOG(info) << "Running: " << bootPath;

	return RunScript(bootPath);
}

/**
 * Generate a Lua subclass of a native class.
 * @param L The Lua state.
 * @param base The name of the native base class.
 * @param name The name of the function (in Lua) generating the class,
 *             used in error messages.
 * @return The number of return values left on the Lua stack.
 */
int RulebookEnv::GenerateSubclass(lua_State *L, const std::string &base,
	const std::string &name)
{
	using namespace luabind;

	RulebookEnv *self = static_cast<RulebookEnv*>(
		lua_touserdata(L, lua_upvalueindex(1)));
	auto &scripting = self->GetScripting();

	auto numParams = lua_gettop(L);
	if (numParams < 1) {
		luaL_error(L, boost::str(
			boost::format("Usage: %s { ... }") % name).c_str());
		return 0;
	}
	else if (numParams > 1) {
		// Pop all but the first param.
		lua_pop(L, -(numParams - 1));
	}

	// defn - The table defining the player.
	// class - The Luabind subclass representation.
	// stage2 - Luabind's class creator closure.
	// base - The Luabind base class representation.
	// ctable - The Lua table representing the subclass.
	// fn - A compiled Lua function.
	// ctor - The compiled constructor function.

	// Initial stack: defn

	void *c = lua_newuserdata(L, sizeof(detail::class_rep));  // defn class
	detail::class_rep *rep =
		new (c)detail::class_rep(L, AutoName().c_str());
	lua_pushvalue(L, -1);  // defn class class

	lua_pushcclosure(L, &detail::create_class::stage2, 1);  // defn class stage2

	// Invoke Luabind's class creator.
	lua_getglobal(L, base.c_str());  // defn class stage2 base
	lua_call(L, 1, 0);  // defn class

	// Define a constructor for the newly-created subclass.
	rep->get_table(L);  // defn class ctable
	scripting.Compile(Script::Core::Chunk(
		boost::str(boost::format(
			"return function(self, peer) %s.__init(self, peer) end") % base),
		boost::str(boost::format("=%s(Internal)") % name)));
	// defn class ctable fn
	scripting.Call();  // defn class ctable ctor
	lua_setfield(L, -2, "__init");  // defn class ctable
	lua_pop(L, 1);  // defn class

	// Initialize the class representation from defn.
	lua_insert(L, -2);  // class defn
	rep->get_table(L);  // class defn ctable
	LuaMergeTables(L, -1, -2);  // class defn ctable

	// Return the class representation.
	lua_pop(L, 2);  // class
	return 1;
}

int RulebookEnv::LPlayer(lua_State *L)
{
	// Player defn
	//
	// Defines a new player class.
	//   defn - A table defining the player:
	//            on_init - (Optional) Set initial properties.

	return GenerateSubclass(L, "MetaPlayer", "Player");
}

int RulebookEnv::LRequire(lua_State *L)
{
	// require(module_name)
	//
	// Loads a module from the current rulebook base directory.
	//   module_name - The name of the module (the filename, minus the ".lua"
	//                 extension.
	//
	// Returns the return value of executing the script.

	RulebookEnv *self = static_cast<RulebookEnv*>(
		lua_touserdata(L, lua_upvalueindex(1)));

	const auto &basePath = self->basePath;

	if (basePath.empty()) {
		HR_LOG(error) <<
			"Rulebook require() called outside of Rulebook context.";
		return 0;
	}

	if (lua_gettop(L) != 1) {
		return luaL_error(L, "Usage: require 'module_name'");
	}

	std::string name = lua_tostring(L, 1);

	if (!IsValidModulePath(name)) {
		return luaL_error(L, "Invalid module filename: %s", name.c_str());
	}
	name += ".lua";

	auto modulePath = basePath;
	modulePath /= Str::UP(name);

	self->requireCache.Push();  // cache
	lua_getfield(L, -1, (const char*)Str::PU(modulePath));  // cache val
	if (!lua_isnil(L, -1)) {
		// Return the cached value.
		lua_remove(L, -2);  // val

		HR_LOG(info) << "Returning cached module '" << name << "' "
			"from: " << modulePath;

		return 1;
	}
	else {
		// Not in the cache.
		lua_pop(L, 1);  // cache

		HR_LOG(info) << "Loading module '" << name << "' "
			"from: " << modulePath;

		try {
			int retv = self->Execute(LoadChunkFromFile(modulePath),
				Core::PassReturn());

			// cache (returns...)
			if (retv >= 1) {
				lua_pushvalue(L, -1 - retv);  // cache (returns...) cache
				lua_remove(L, -2 - retv);  // (returns...) cache
				// For now, we only save the first return value.
				lua_pushvalue(L, -1 - retv);  // (returns...) cache ret
				lua_setfield(L, -2,
					(const char*)Str::PU(modulePath)); // (returns...) cache
				lua_pop(L, 1);  // (returns...)
			}

			return retv;
		}
		catch (Script::ScriptExn &ex) {
			HR_LOG(error) << ex.what();
			return 0;
		}
	}
}

int RulebookEnv::LRulebookStage1(lua_State *L)
{
	// Rulebook name defn
	//
	// Defines a new rulebook.
	//   name - The name of the rulebook.
	//   defn - A table defining the rulebook:
	//            title - The title.
	//            description - (Optional) The one-line description.
	//            on_pre_game - (Optional) Function to call before the session starts.
	//            on_post_game - (Optional) Function to call after the session ends.

	if (lua_gettop(L) != 1) {
		luaL_error(L, "Usage: Rulebook 'name' { ... }");
		return 0;
	}

	// This is a little syntax trick to make rulebooks look declarative.
	// This stage captures the name of the rulebook, then returns the function
	// (stage 2) that will combine it with the object.
	// Inspired by Luabind's class definition mechanism.

	lua_pushvalue(L, lua_upvalueindex(1));  // name self
	lua_pushcclosure(L, &RulebookEnv::LRulebookStage2, 2);  // fn
	return 1;
}

int RulebookEnv::LRulebookStage2(lua_State *L)
{
	using namespace luabind;

	if (lua_gettop(L) != 1) {
		luaL_error(L, "Usage: Rulebook 'name' { ... }");
		return 0;
	}

	const char *name = lua_tostring(L, lua_upvalueindex(1));
	RulebookEnv *self = static_cast<RulebookEnv*>(lua_touserdata(L, lua_upvalueindex(2)));

	if (!name) {
		luaL_error(L, "Rulebook name must be a string.");
		return 0;
	}

	self->DefineRulebook(name, object(from_stack(L, 1)));

	return 0;
}

int RulebookEnv::LSession(lua_State *L)
{
	// Session defn
	//
	// Defines a new session class.
	//   defn - A table defining the session:
	//            on_init - (Optional) Set initial properties.

	return GenerateSubclass(L, "MetaSession", "Session");
}

}  // namespace HoverScript
}  // namespace Client
}  // namespace HoverRace


// Handlers.h
//
// Copyright (c) 2010, 2013, 2014 Michael Imamura.
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

#include <luabind/object.hpp>

#include "RegistryRef.h"

#if defined(_WIN32) && defined(HR_ENGINE_SHARED)
#	ifdef MR_ENGINE
#		define MR_DllDeclare   __declspec( dllexport )
#	else
#		define MR_DllDeclare   __declspec( dllimport )
#	endif
#else
#	define MR_DllDeclare
#endif

namespace HoverRace {
namespace Script {

class Core;

/**
 * Tracks event handlers registered in scripts.
 * @author Michael Imamura
 */
class MR_DllDeclare Handlers
{
public:
	Handlers(Core &scripting);
	Handlers(const Handlers&) = default;
	Handlers(Handlers&&) = default;
	virtual ~Handlers() { }

	Handlers &operator=(const Handlers&) = default;
	Handlers &operator=(Handlers&&) = default;

protected:
	void Call(int numParams) const;
public:
	void CallHandlers() const;
	void CallHandlers(const luabind::object &p1) const;
	void CallHandlers(const luabind::object &p1,
		const luabind::object &p2) const;

	void AddHandler(const luabind::object &fn);
	void AddHandler(const std::string &name, const luabind::object &fn);

private:
	Core *scripting;
	int seq;
	RegistryRef ref;
};

}  // namespace Script
}  // namespace HoverRace

#undef MR_DllDeclare

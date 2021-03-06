
// ScriptExn.h
//
// Copyright (c) 2009, 2014 Michael Imamura.
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

/**
 * Generic script engine exception.
 * @author Michael Imamura
 */
class MR_DllDeclare ScriptExn : public std::exception
{
	using SUPER = std::exception;

public:
	ScriptExn() : SUPER() { }
	ScriptExn(const char* const msg) : SUPER(), msg(msg) { }
	ScriptExn(const std::string &msg) : SUPER(), msg(msg) { }
	virtual ~ScriptExn() noexcept { }

	const char* what() const noexcept override { return msg.c_str(); }
	virtual const std::string &GetMessage() const noexcept { return msg; }

private:
	std::string msg;
};

}  // namespace Script
}  // namespace HoverRace

#undef MR_DllDeclare

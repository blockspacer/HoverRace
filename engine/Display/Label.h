
// Label.h
//
// Copyright (c) 2013-2016 Michael Imamura.
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

#include "BaseText.h"

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
	namespace Display {
		class Display;
	}
}

namespace HoverRace {
namespace Display {

/**
 * A static text label.
 * This is intended for text elements that rarely change over time, aside from
 * visibility.
 * @author Michael Imamura
 */
class MR_DllDeclare Label : public BaseText
{
	using SUPER = BaseText;

public:
	struct Props
	{
		enum
		{
			SCALE = SUPER::Props::NEXT_,
			WRAP_WIDTH,  ///< Fired when a fixed width is set or auto-width is enabled.
			NEXT_,  ///< First index for subclasses.
		};
	};

public:
	Label(const std::string &text, const UiFont &font, const Color color,
		uiLayoutFlags_t layoutFlags = 0);
	Label(double wrapWidth, const std::string &text, const UiFont &font,
		const Color color, uiLayoutFlags_t layoutFlags = 0);
	virtual ~Label() { }

public:
	void AttachView(Display &disp) override { AttachViewDynamic(disp, this); }

public:
	/**
	 * Check if automatic width sizing is enabled.
	 * @return @c true if the width is determined by the longest line in
	 *         the text, or @c false if the text is wrapped to fit a
	 *         specific width.
	 */
	bool IsAutoWidth() const { return wrapWidth <= 0; }
	void SetAutoWidth();

	/**
	 * Gets the scaling of this label.
	 * @return The scale.
	 */
	double GetScale() const { return scale; }
	void SetScale(const double scale);

	/**
	 * Returns the set width, if a fixed width is set.
	 * @return The width.  If auto-width is enabled, the result is
	 *         undefined (always check IsAutoWidth() first).
	 */
	double GetWrapWidth() const { return wrapWidth; }
	void SetWrapWidth(double wrapWidth);

private:
	double scale;
	double wrapWidth;
};

}  // namespace Display
}  // namespace HoverRace

#undef MR_DllDeclare

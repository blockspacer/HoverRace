
// SdlWallpaperView.h
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

#include "SdlDisplay.h"
#include "SdlView.h"

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
		class Wallpaper;
	}
}

namespace HoverRace {
namespace Display {
namespace SDL {

/**
 * SDL view for Wallpaper.
 * @author Michael Imamura
 */
class MR_DllDeclare SdlWallpaperView : public SdlView<Wallpaper>
{
	typedef SdlView<Wallpaper> SUPER;
	public:
		SdlWallpaperView(SdlDisplay &disp, Wallpaper &model);
		virtual ~SdlWallpaperView() { }

	public:
		void OnModelUpdate(int prop) override;

	public:
		Vec3 Measure() override;
		void PrepareRender() override;
		void Render() override;

	private:
		void Update();

	private:
		bool fillChanged;
		bool opacityChanged;
		MR_UInt8 computedAlpha;
		SDL_Rect destRect;
		const SDL_Rect *destRectPtr;
		std::shared_ptr<SdlTexture> texture;
		boost::signals2::connection displayConfigChangedConn;
};

}  // namespace SDL
}  // namespace Display
}  // namespace HoverRace

#undef MR_DllDeclare

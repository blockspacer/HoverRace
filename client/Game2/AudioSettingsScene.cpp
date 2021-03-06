
// AudioSettingsScene.cpp
//
// Copyright (c) 2014-2016 Michael Imamura.
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

#include "../../engine/Display/Slider.h"
#include "../../engine/ObjFac1/ObjFac1Res.h"
#include "../../engine/ObjFacTools/ResourceLib.h"
#include "../../engine/Parcel/ResBundle.h"
#include "../../engine/Util/Config.h"
#include "../../engine/Util/Log.h"
#include "../../engine/VideoServices/SoundServer.h"

#include "AudioSettingsScene.h"

using namespace HoverRace::Util;
namespace SoundServer = HoverRace::VideoServices::SoundServer;

namespace HoverRace {
namespace Client {

AudioSettingsScene::AudioSettingsScene(Display::Display &display,
	GameDirector &director, const std::string &parentTitle) :
	SUPER(display, director, parentTitle, _("AUDIO"), "Audio Settings"),
	audioCfg(Config::GetInstance()->audio), origAudioCfg(audioCfg),
	skipSoundTest(true),
	testSound(LoadSound(MR_SND_START))
{
	using namespace Display;

	sfxVolumeSlider = AddSetting(_("Sound Effects")).
		NewChild<Slider>(display, 0, 1.0, 0.1)->GetContents();
	sfxVolumeSlider->SetSize(SLIDER_SIZE);
	sfxVolumeConn = sfxVolumeSlider->GetValueChangedSignal().connect([&](double val) {
		audioCfg.sfxVolume = val;
		if (skipSoundTest) {
			skipSoundTest = false;
		}
		else {
			SoundServer::Play(testSound);
		}
	});

	GetSettingsGrid()->RequestFocus();
}

void AudioSettingsScene::LoadFromConfig()
{
	// Don't play the sound test when first loading.
	skipSoundTest = true;

	sfxVolumeSlider->SetValue(audioCfg.sfxVolume);
}

void AudioSettingsScene::ResetToDefaults()
{
	audioCfg.ResetToDefaults();
}

/**
 * Loads a sound from the resources.
 * @param id The resource ID.
 * @return The sound effect, or @c nullptr if it fails to load.
 */
VideoServices::ShortSound *AudioSettingsScene::LoadSound(int id)
{
	auto cfg = Config::GetInstance();
	auto res = cfg->GetResBundle().GetResourceLib().GetShortSound(id);
	if (!res) {
		HR_LOG(warning) << "Unable to load sound ID: " << id;
		return nullptr;
	}

	return res->GetSound();
}

void AudioSettingsScene::OnOk()
{
	Config::GetInstance()->Save();
	SUPER::OnOk();
}

void AudioSettingsScene::OnCancel()
{
	audioCfg = origAudioCfg;
	SUPER::OnCancel();
}

}  // namespace Client
}  // namespace HoverRace

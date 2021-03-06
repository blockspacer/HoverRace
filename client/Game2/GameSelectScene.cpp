
// GameSelectScene.cpp
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

#include "../../engine/Control/Controller.h"
#include "../../engine/Display/Button.h"
#include "../../engine/Display/Container.h"
#include "../../engine/Display/Display.h"
#include "../../engine/Display/FlexGrid.h"
#include "../../engine/Display/Label.h"
#include "../../engine/Util/Config.h"
#include "../../engine/Util/Log.h"

#include "Rulebook.h"
#include "RulebookLibrary.h"
#include "Rules.h"
#include "TrackSelectScene.h"

#include "GameSelectScene.h"

using namespace HoverRace::Util;

namespace HoverRace {
namespace Client {

namespace {

const double BTN_WIDTH = 240;
const double BTN_HEIGHT = 520;
const double BTN_GAP = 20;

const Vec2 BTN_SIZE{ BTN_WIDTH, BTN_HEIGHT };

class ModeButton : public Display::Button
{
	using SUPER = Display::Button;

public:
	ModeButton(Display::Display &display,
		std::shared_ptr<const Rulebook> rulebook) :
		SUPER(display, BTN_SIZE, ""),
		contentRoot(std::make_shared<Display::Container>(display))
	{
		using namespace Display;

		contentRoot->AttachView(display);

		labelGrid = contentRoot->NewChild<FlexGrid>(display);
		labelGrid->SetPos(20, 320);
		labelGrid->SetMargin(0, 10);

		const auto &s = display.styles;
		const auto wrapWidth = BTN_SIZE.x - 40.0;

		size_t row = 0;
		labelGrid->At(row++, 1).NewChild<Label>(
			rulebook->GetTitle(), s.headingFont, s.headingFg)->
			GetContents()->SetWrapWidth(wrapWidth);
		labelGrid->At(row++, 1).NewChild<Label>(
			rulebook->GetDescription(), s.bodyFont, s.bodyFg)->
			GetContents()->SetWrapWidth(wrapWidth);

		SetContents(contentRoot);
	}

	virtual ~ModeButton() { }

private:
	std::shared_ptr<Display::Container> contentRoot;
	std::shared_ptr<Display::FlexGrid> labelGrid;
};

}  // namespace

GameSelectScene::GameSelectScene(Display::Display &display,
	GameDirector &director, RulebookLibrary &rulebookLibrary,
	bool multiplayer) :
	SUPER(display, director, "", "Rulebook Select"),
	display(display), director(director),
	trackSelected(false)
{
	using namespace Display;

	SetPhaseTransitionDuration(1000);
	SetStateTransitionDuration(1000);

	SetStoppingTransitionEnabled(true);
	SetBackground(nullptr);

	SupportCancelAction(_("Back"));

	auto root = GetContentRoot();

	// Build the list of valid rulebooks.
	std::vector<std::shared_ptr<const Rulebook>> rulebooks;
	for (auto iter = rulebookLibrary.cbegin();
		iter != rulebookLibrary.cend(); ++iter)
	{
		auto rulebook = *iter;
		if (!multiplayer || rulebook->GetMaxPlayers() > 1) {
			rulebooks.push_back(rulebook);
		}
	}

	double numRulebooks = static_cast<double>(rulebooks.size());
	double panelWidth = (numRulebooks * (BTN_WIDTH + BTN_GAP)) - BTN_GAP;
	rulebookPanel = root->NewChild<Container>(display,
		Vec2(panelWidth, BTN_HEIGHT));
	rulebookPanel->SetPos(DialogScene::MARGIN_WIDTH, 0);
	rulebookPanel->SetClip(false);

	const Vec2 btnSize(BTN_WIDTH, BTN_HEIGHT);
	double x = 0;
	for (auto rulebook : rulebooks) {
		auto btn = rulebookPanel->NewChild<ModeButton>(display, rulebook);
		btn->SetPos(x, 0);
		btn->GetClickedSignal().connect(std::bind(
			&GameSelectScene::OnRulebookSelected, this, rulebook));

		modeBtns.emplace_back(std::move(btn));

		x += BTN_WIDTH + BTN_GAP;
	}

	rulebookPanel->RequestFocus();
}

GameSelectScene::~GameSelectScene()
{
}

void GameSelectScene::OnRulebookSelected(std::shared_ptr<const Rulebook> rulebook)
{
	auto scene = std::make_shared<TrackSelectScene>(display, director, rulebook);
	scene->GetOkSignal().connect([&](std::shared_ptr<Rules> rules,
		std::shared_ptr<Display::Res<Display::Texture>> mapRes)
	{
		trackSelected = true;
		director.RequestPopScene();
		okSignal(rules, mapRes);
	});
	scene->GetCancelSignal().connect([&]() {
		director.RequestPopScene();
		cancelSignal();
	});
	director.RequestPushScene(scene);
}

void GameSelectScene::OnOk()
{
}

void GameSelectScene::OnCancel()
{
	cancelSignal();
}

void GameSelectScene::OnPhaseTransition(double progress)
{
	double f = pow((1.0 - progress), 4);

	rulebookPanel->SetTranslation(f * 1280.0, 0);

	double offset = 0;
	for (auto &btn : modeBtns) {
		btn->SetTranslation(f * offset, 0);
		offset += 600.0;
	}

	SUPER::OnPhaseTransition(progress);
}

void GameSelectScene::OnStateTransition(double progress)
{
	if (GetPhase() != Phase::RUNNING) return;

	double f =
		GetState() == State::LOWERING ?
		1 - pow(progress, 4) :
		pow(1.0 - progress, 4);

	rulebookPanel->SetTranslation(0, f * -(BTN_HEIGHT + 1));

	SUPER::OnStateTransition(progress);
}

void GameSelectScene::Render()
{
	// If we're exiting because a track was selected, then don't render
	// anything since the track select scene will be sliding up.
	if (!trackSelected) {
		SUPER::Render();
	}
}

}  // namespace Client
}  // namespace HoverRace

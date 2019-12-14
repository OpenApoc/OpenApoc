#include "game/ui/base/basestage.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/gamestate.h"
#include "library/strings_format.h"

namespace OpenApoc
{

BaseStage::BaseStage(sp<GameState> state)
    : Stage(), viewHighlight(BaseGraphics::FacilityHighlight::None), state(state)
{
}

BaseStage::~BaseStage() = default;

void BaseStage::changeBase(sp<Base> newBase)
{
	if (newBase != nullptr)
	{
		state->current_base = {state.get(), newBase};
	}
}

void BaseStage::refreshView()
{
	auto viewImage = drawMiniBase(*state->current_base, viewHighlight, viewFacility);
	currentView->setImage(viewImage);
	currentView->setDepressedImage(viewImage);
}

void BaseStage::begin()
{
	changeBase();

	textFunds = form->findControlTyped<Label>("TEXT_FUNDS");
	textFunds->setText(state->getPlayerBalance());

	int b = 0;
	for (auto &pair : state->player_bases)
	{
		auto &viewBase = pair.second;
		auto viewName = format("BUTTON_BASE_%d", ++b);
		auto view = form->findControlTyped<GraphicButton>(viewName);
		if (!view)
		{
			// This screen doesn't have miniviews
			return;
		}
		if (state->current_base == viewBase)
		{
			currentView = view;
		}
		view->setData(viewBase);
		auto viewImage = drawMiniBase(*viewBase, viewHighlight, viewFacility);
		view->setImage(viewImage);
		view->setDepressedImage(viewImage);
		wp<GraphicButton> weakView(view);
		view->addCallback(FormEventType::ButtonClick, [this, weakView](FormsEvent *e) {
			auto base = e->forms().RaisedBy->getData<Base>();
			if (this->state->current_base != base)
			{
				this->changeBase(base);
				this->currentView = weakView.lock();
			}
		});
		view->addCallback(FormEventType::MouseEnter, [this](FormsEvent *e) {
			auto base = e->forms().RaisedBy->getData<Base>();
			this->textViewBase->setVisible(true);
			this->textViewBase->setText(base->name);
		});
		view->addCallback(FormEventType::MouseLeave, [this](FormsEvent *) {
			this->textViewBase->setText("");
			this->textViewBase->setVisible(false);
		});
		miniViews.push_back(view);
	}
	textViewBase = form->findControlTyped<Label>("TEXT_BUTTON_BASE");
	this->textViewBase->setVisible(false);
}

void BaseStage::render()
{
	// Highlight selected base
	if (currentView != nullptr)
	{
		auto viewBase = currentView->getData<Base>();
		if (state->current_base == viewBase)
		{
			Vec2<int> pos = currentView->getLocationOnScreen() - 2;
			Vec2<int> size = currentView->Size + 4;
			fw().renderer->drawRect(pos, size, COLOUR_RED);
		}
	}
}

}; // namespace OpenApoc

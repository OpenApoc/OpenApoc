#include "game/state/base/base.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/ui/base/basestage.h"

namespace OpenApoc
{

BaseStage::BaseStage(sp<GameState> state)
    : Stage(), viewHighlight(BaseGraphics::FacilityHighlight::None), state(state)
{
}

BaseStage::~BaseStage() = default;

void BaseStage::ChangeBase(sp<Base> newBase)
{
	if (newBase != nullptr)
	{
		state->current_base = {state.get(), newBase};
	}
}

void BaseStage::RefreshView()
{
	auto viewImage = drawMiniBase(state->current_base, viewHighlight, viewFacility);
	currentView->SetImage(viewImage);
	currentView->SetDepressedImage(viewImage);
}

void BaseStage::Begin()
{
	ChangeBase();

	textFunds = form->FindControlTyped<Label>("TEXT_FUNDS");
	textFunds->SetText(state->getPlayerBalance());

	int b = 0;
	for (auto &pair : state->player_bases)
	{
		auto &viewBase = pair.second;
		auto viewName = UString::format("BUTTON_BASE_%d", ++b);
		auto view = form->FindControlTyped<GraphicButton>(viewName);
		if (!view)
		{
			// This screen doesn't have miniviews
			return;
		}
		if (state->current_base == viewBase)
		{
			currentView = view;
		}
		view->SetData(viewBase);
		auto viewImage = drawMiniBase(viewBase, viewHighlight, viewFacility);
		view->SetImage(viewImage);
		view->SetDepressedImage(viewImage);
		view->addCallback(FormEventType::ButtonClick, [this, view](Event *e) {
			auto base = e->Forms().RaisedBy->GetData<Base>();
			if (this->state->current_base != base)
			{
				this->ChangeBase(base);
				this->currentView = view;
			}
		});
		view->addCallback(FormEventType::MouseEnter, [this](Event *e) {
			auto base = e->Forms().RaisedBy->GetData<Base>();
			this->textViewBase->SetText(base->name);
		});
		view->addCallback(FormEventType::MouseLeave,
		                  [this](Event *) { this->textViewBase->SetText(""); });
		miniViews.push_back(view);
	}
	textViewBase = form->FindControlTyped<Label>("TEXT_BUTTON_BASE");
}

void BaseStage::Render()
{
	// Highlight selected base
	if (currentView != nullptr)
	{
		auto viewBase = currentView->GetData<Base>();
		if (state->current_base == viewBase)
		{
			Vec2<int> pos = form->Location + currentView->Location - 2;
			Vec2<int> size = currentView->Size + 4;
			fw().renderer->drawRect(pos, size, Colour{255, 0, 0});
		}
	}
}

}; // namespace OpenApoc

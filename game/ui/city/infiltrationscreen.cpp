#include "game/ui/city/infiltrationscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include "library/line.h"
#include <array>

namespace OpenApoc
{

// The image seems to have 6 segments, each split into 7, so it's meant to be 6 weeks of values?
constexpr int num_steps = 6 * 7;

static void drawOrgLine(sp<RGBImage> image, const Organisation &org, const Colour &colour,
                        int steps)
{
	const float step_width = static_cast<float>(image->size.x - 1) / static_cast<float>(steps);
	constexpr int max_infiltration_value = 100;
	float infiltration_y_scale =
	    static_cast<float>(image->size.y - 1) / static_cast<float>(max_infiltration_value);

	// Initialise all steps to zero, in case there's not enough history (we assume anything
	// pre-game-start has 0 infilation anyway)
	auto step_values = std::vector<float>(steps, 0.0f);
	// First is always current infiltration value
	step_values[0] = static_cast<float>(std::min(max_infiltration_value, org.infiltrationValue));
	int step = 1;
	for (const auto step_value : org.infiltrationHistory)
	{
		if (step > steps)
			return;
		step_values[step] = static_cast<float>(std::min(max_infiltration_value, step_value));
		step++;
	}

	auto image_lock = RGBImageLock(image);

	// TODO: Make curved lines
	for (step = 0; step < steps - 1; step++)
	{
		const Vec3<float> start_point = {
		    static_cast<float>(image->size.x - 1) - static_cast<float>(step) * step_width,
		    static_cast<float>(image->size.y - 1) - step_values[step] * infiltration_y_scale, 0};
		const Vec3<float> end_point = {
		    static_cast<float>(image->size.x - 1) - static_cast<float>(step + 1) * step_width,
		    static_cast<float>(image->size.y - 1) - step_values[step + 1] * infiltration_y_scale,
		    0};

		const auto start_point_int = Vec3<int>(start_point);
		const auto end_point_int = Vec3<int>(end_point);

		const auto line = LineSegment<int, false>(start_point_int, end_point_int);
		for (auto point : line)
		{
			if (point.x < 0 || point.y < 0 || point.x >= image->size.x || point.y >= image->size.y)
			{
				LogWarning("Point %s out of bounds for image of size %s", point, image->size);
				point.x = clamp(point.x, 0, static_cast<int>(image->size.x - 1));
				point.y = clamp(point.y, 0, static_cast<int>(image->size.y - 1));
			}
			image_lock.set(Vec2<unsigned int>{static_cast<unsigned int>(point.x),
			                                  static_cast<unsigned int>(point.y)},
			               colour);
		}
	}
}

constexpr std::array<Colour, 10> line_colors = {
    Colour{195, 47, 47},  Colour{235, 79, 27}, Colour{243, 171, 87}, Colour{232, 247, 139},
    Colour{235, 213, 25}, Colour{24, 231, 24}, Colour{55, 145, 72},  Colour{5, 101, 255},
    Colour{54, 74, 200},  Colour{143, 15, 178}};

InfiltrationScreen::InfiltrationScreen(sp<GameState> state)
    : Stage(), menuform(ui().getForm("city/infiltration")), state(state)
{
	for (int i = 0; i < 10; i++)
	{
		shown_org_names[i] = menuform->findControlTyped<Label>(format("ORG_NAME_%d", i)).get();
		shown_orgs[i] = nullptr;
	}
	graph = menuform->findControlTyped<Graphic>("GRAPH");
}

InfiltrationScreen::~InfiltrationScreen() = default;

void InfiltrationScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	this->update_view();
}

void InfiltrationScreen::pause() {}

void InfiltrationScreen::resume() {}

void InfiltrationScreen::finish() {}

void InfiltrationScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_QUIT")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_TOPTEN")
		{
			this->reset_shown_orgs();
			return;
		}
	}
}

void InfiltrationScreen::reset_shown_orgs()
{
	std::vector<const Organisation *> orgs;

	for (const auto &org : state->organisations)
	{
		if (org.second->id == "ORG_ALIEN" || org.second->id == "ORG_X-COM")
			continue;
		if (org.second->infiltrationValue == 0)
			continue;
		orgs.push_back(org.second.get());
	}

	auto infiltration_comparer = [](const Organisation *org1, const Organisation *org2) {
		return org1->infiltrationValue > org2->infiltrationValue;
	};
	std::sort(orgs.begin(), orgs.end(), infiltration_comparer);

	for (int i = 0; i < 10; i++)
	{
		if (i >= orgs.size())
		{
			shown_orgs[i] = nullptr;
			continue;
		}
		shown_orgs[i] = orgs[i];
	}
	this->update_view();
}

void InfiltrationScreen::update_view()
{
	auto newGraphImage = mksp<RGBImage>(graph->Size);

	for (int i = 0; i < 10; i++)
	{
		if (shown_orgs[i])
		{
			shown_org_names[i]->setText(shown_orgs[i]->name);
			drawOrgLine(newGraphImage, *shown_orgs[i], line_colors[i], num_steps);
		}
		else
		{
			shown_org_names[i]->setText("");
		}
	}

	graph->setImage(newGraphImage);
}

void InfiltrationScreen::update() { menuform->update(); }

void InfiltrationScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool InfiltrationScreen::isTransition() { return false; }

}; // namespace OpenApoc

#include "form.h"
#include "forms/harness_actions.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/data.h"
#include "framework/framework.h"

namespace OpenApoc
{

namespace
{
// Forms register themselves on construction so the harness can enumerate what is on screen.
std::vector<Form *> g_liveForms;
} // namespace

const std::vector<Form *> &Form::liveForms() { return g_liveForms; }

Form::Form() : Control()
{
	g_liveForms.push_back(this);
	// Installed from here rather than at startup so the action handler exists as soon as there is
	// any form to act on. Without it CONTROL/ACTION/HELP answer "no action handler".
	installFormsHarnessActions();
}

Form::~Form()
{
	g_liveForms.erase(std::remove(g_liveForms.begin(), g_liveForms.end(), this), g_liveForms.end());
}

void Form::readFormStyle(pugi::xml_node *node)
{
	if (node == nullptr)
	{
		return;
	}
	UString nodename;
	this->Name = node->attribute("id").as_string();

	for (auto child = node->first_child(); child; child = child.next_sibling())
	{
		nodename = child.name();
		if (nodename == "style")
		{
			// TODO: Determine best "style" based on minwidth and minheight attributes
			configureFromXml(&child);
			resolveLocation();
		}
	}
}

void Form::eventOccured(Event *e) { Control::eventOccured(e); }

void Form::onRender()
{
	// A form that renders or updates is on screen. That is what the harness means by "visible" --
	// liveForms() knows every constructed form, but only these are actually being shown, and an
	// automated driver must not click a control belonging to a screen nobody can see.
	if (auto self = std::dynamic_pointer_cast<Form>(weak_from_this().lock()))
	{
		notifyVisibleForm(self);
	}
	Control::onRender();
}

void Form::update()
{
	if (auto self = std::dynamic_pointer_cast<Form>(weak_from_this().lock()))
	{
		notifyVisibleForm(self);
	}
	Control::update();
	resolveLocation();
}

void Form::unloadResources() { Control::unloadResources(); }

sp<Control> Form::copyTo(sp<Control> CopyParent)
{
	sp<Form> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<Form>();
	}
	else
	{
		copy = mksp<Form>();
	}
	copyControlData(copy);
	return copy;
}

sp<Form> Form::loadForm(const UString &path)
{
	auto file = fw().data->fs.open(path);
	if (!file)
	{
		LogWarning("Failed to open form file \"{0}\"", path);
		return nullptr;
	}
	auto data = file.readAll();
	if (!data)
	{
		LogWarning("Failed to read form data from \"{0}\"", path);
		return nullptr;
	}
	pugi::xml_document doc;
	auto result = doc.load_buffer(data.get(), file.size());
	if (!result)
	{
		LogWarning("Failed to parse form file at \"{0}\" - \"{1}\" at \"{2}\"", path,
		           result.description(), (unsigned long long)result.offset);
		return nullptr;
	}

	auto node = doc.child("openapoc");
	if (!node)
	{
		LogWarning("No root \"openapoc\" root element in form file \"{0}\"", path);
		return nullptr;
	}
	auto child = node.child("form");
	if (!child)
	{
		LogWarning("No child node of \"form\" in form file \"{0}\"", path);
		return nullptr;
	}
	auto form = mksp<Form>();
	form->readFormStyle(&child);
	return form;
}

}; // namespace OpenApoc

#include "form.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/data.h"
#include "framework/framework.h"

namespace OpenApoc
{

Form::Form() : Control() {}

Form::~Form() = default;

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

void Form::onRender() { Control::onRender(); }

void Form::update()
{
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
		LogWarning("Failed to open form file \"%s\"", path);
		return nullptr;
	}
	auto data = file.readAll();
	if (!data)
	{
		LogWarning("Failed to read form data from \"%s\"", path);
		return nullptr;
	}
	pugi::xml_document doc;
	auto result = doc.load_buffer(data.get(), file.size());
	if (!result)
	{
		LogWarning("Failed to parse form file at \"%s\" - \"%s\" at \"%llu\"", path,
		           result.description(), (unsigned long long)result.offset);
		return nullptr;
	}

	auto node = doc.child("openapoc");
	if (!node)
	{
		LogWarning("No root \"openapoc\" root element in form file \"%s\"", path);
		return nullptr;
	}
	auto child = node.child("form");
	if (!child)
	{
		LogWarning("No child node of \"form\" in form file \"%s\"", path);
		return nullptr;
	}
	auto form = mksp<Form>();
	form->readFormStyle(&child);
	return form;
}

}; // namespace OpenApoc

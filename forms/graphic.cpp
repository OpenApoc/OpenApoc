#include "forms/graphic.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "library/sp.h"

namespace OpenApoc
{

Graphic::Graphic(sp<Image> Image)
    : Control(), image(Image), ImageHAlign(HorizontalAlignment::Left),
      ImageVAlign(VerticalAlignment::Top), ImagePosition(FillMethod::Fit), AutoSize(false)
{
}

Graphic::~Graphic() = default;

void Graphic::eventOccured(Event *e) { Control::eventOccured(e); }

void Graphic::onRender()
{
	Control::onRender();

	if (!image)
	{
		return;
	}

	Vec2<float> pos = {0, 0};
	if (Vec2<unsigned int>(Size) == image->size)
	{
		fw().renderer->draw(image, pos);
	}
	else
	{
		switch (ImagePosition)
		{
			case FillMethod::Stretch:
				fw().renderer->drawScaled(image, pos, Size);
				break;

			case FillMethod::Fit:
				switch (ImageVAlign)
				{
					case VerticalAlignment::Top:
						pos.y = 0;
						break;
					case VerticalAlignment::Centre:
						pos.y = (Size.y - image->size.y) / 2;
						break;
					case VerticalAlignment::Bottom:
						pos.y = Size.y - image->size.y;
						break;
					default:
						LogError("Unknown ImageVAlign");
						return;
				}

				switch (ImageHAlign)
				{
					case HorizontalAlignment::Left:
						pos.x = 0;
						break;
					case HorizontalAlignment::Centre:
						pos.x = (Size.x - image->size.x) / 2;
						break;
					case HorizontalAlignment::Right:
						pos.x = Size.x - image->size.x;
						break;
					default:
						LogError("Unknown ImageHAlign");
						return;
				}

				fw().renderer->draw(image, pos);
				break;

			case FillMethod::Tile:
				for (pos.x = 0; pos.x < Size.x; pos.x += image->size.x)
				{
					for (pos.y = 0; pos.y < Size.y; pos.y += image->size.y)
					{
						fw().renderer->draw(image, pos);
					}
				}
				break;
		}
	}
}

void Graphic::update()
{
	Control::update();

	if (image && AutoSize)
	{
		Size = image->size;
	}
}

void Graphic::unloadResources()
{
	image.reset();
	Control::unloadResources();
}

sp<Image> Graphic::getImage() const { return image; }

void Graphic::setImage(sp<Image> Image)
{
	image = Image;
	this->setDirty();
}

sp<Control> Graphic::copyTo(sp<Control> CopyParent)
{
	sp<Graphic> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<Graphic>(this->image);
	}
	else
	{
		copy = mksp<Graphic>(this->image);
	}
	copy->ImageHAlign = this->ImageHAlign;
	copy->ImageVAlign = this->ImageVAlign;
	copy->ImagePosition = this->ImagePosition;
	copy->AutoSize = this->AutoSize;
	copyControlData(copy);
	return copy;
}

void Graphic::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);

	auto imageNode = node->child("image");
	if (imageNode)
	{
		image = fw().data->loadImage(imageNode.text().get());
	}
	auto alignNode = node->child("alignment");
	if (alignNode)
	{
		UString hAlign = alignNode.attribute("horizontal").as_string();
		if (hAlign == "left")
		{
			ImageHAlign = HorizontalAlignment::Left;
		}
		else if (hAlign == "centre")
		{
			ImageHAlign = HorizontalAlignment::Centre;
		}
		else if (hAlign == "right")
		{
			ImageHAlign = HorizontalAlignment::Right;
		}
		UString vAlign = alignNode.attribute("vertical").as_string();
		if (vAlign == "top")
		{
			ImageVAlign = VerticalAlignment::Top;
		}
		else if (vAlign == "centre")
		{
			ImageVAlign = VerticalAlignment::Centre;
		}
		else if (vAlign == "bottom")
		{
			ImageVAlign = VerticalAlignment::Bottom;
		}
	}
	auto imagePositionNode = node->child("imageposition");
	if (imagePositionNode)
	{
		UString position = imagePositionNode.text().get();
		if (position == "stretch")
		{
			ImagePosition = FillMethod::Stretch;
		}
		else if (position == "fit")
		{
			ImagePosition = FillMethod::Fit;
		}
		else if (position == "tile")
		{
			ImagePosition = FillMethod::Tile;
		}
	}
	auto autoSizeNode = node->child("autosize");
	if (autoSizeNode)
	{
		AutoSize = autoSizeNode.text().as_bool();
	}
}
}; // namespace OpenApoc

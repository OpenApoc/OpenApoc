#include "library/sp.h"
#include "forms/graphic.h"
#include "game/resources/gamecore.h"
#include "framework/framework.h"

namespace OpenApoc
{

Graphic::Graphic(Framework &fw, Control *Owner, UString Image)
	: Control(fw, Owner), ImageHAlign(HorizontalAlignment::Left), ImageVAlign(VerticalAlignment::Top),
	ImagePosition(FillMethod::Stretch), AutoSize(false)
{
	image_name = Image;
	image = fw.gamecore->GetImage(image_name);
}

Graphic::~Graphic() {}

void Graphic::EventOccured(Event *e) { Control::EventOccured(e); }

void Graphic::OnRender()
{
	if (!image)
	{
		image = fw.gamecore->GetImage(image_name);
		if (!image)
		{
			return;
		}
	}

	if (AutoSize)
	{
		Size = image->size;
	}

	Vec2<float> pos = {0, 0};
	if (Vec2<unsigned int>(Size) == image->size)
	{
		fw.renderer->draw(image, pos);
	}
	else
	{
		switch (ImagePosition)
		{
		case FillMethod::Stretch:
			fw.renderer->drawScaled(image, pos, Size);
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

			fw.renderer->draw(image, pos);
			break;

		case FillMethod::Tile:
			for (pos.x = 0; pos.x < Size.x; pos.x += image->size.x)
			{
				for (pos.y = 0; pos.y < Size.y; pos.y += image->size.y)
				{
					fw.renderer->draw(image, pos);
				}
			}
			break;
		}
	}
}

void Graphic::Update() { Control::Update(); }

void Graphic::UnloadResources()
{
	image.reset();
	Control::UnloadResources();
}

sp<Image> Graphic::GetImage() { return image; }

void Graphic::SetImage(sp<Image> Image) { image = Image; }

Control *Graphic::CopyTo(Control *CopyParent)
{
	Graphic *copy = new Graphic(fw, CopyParent, image_name);
	copy->ImageHAlign = this->ImageHAlign;
	copy->ImageVAlign = this->ImageVAlign;
	copy->ImagePosition = this->ImagePosition;
	copy->AutoSize = this->AutoSize;
	CopyControlData((Control *)copy);
	return (Control *)copy;
}

}; // namespace OpenApoc

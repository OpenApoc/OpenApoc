#include "library/sp.h"

#include "forms/graphicbutton.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

GraphicButton::GraphicButton(Framework &fw, Control *Owner, UString Image, UString ImageDepressed)
    : Control(fw, Owner)
{
	image = nullptr;
	imagedepressed = nullptr;
	imagehover = nullptr;
	image_name = Image;
	imagedepressed_name = ImageDepressed;
	imagehover_name = "";
	this->buttonclick =
	    fw.data->load_sample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050");
}

GraphicButton::GraphicButton(Framework &fw, Control *Owner, UString Image, UString ImageDepressed,
                             UString ImageHover)
    : Control(fw, Owner)
{
	image = nullptr;
	imagedepressed = nullptr;
	imagehover = nullptr;
	image_name = Image;
	imagedepressed_name = ImageDepressed;
	imagehover_name = ImageHover;
	this->buttonclick =
	    fw.data->load_sample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050");
}

GraphicButton::~GraphicButton() {}

void GraphicButton::EventOccured(Event *e)
{
	Control::EventOccured(e);

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseDown)
	{
		fw.soundBackend->playSample(buttonclick);
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseClick)
	{
		auto ce = new Event();
		ce->Type = e->Type;
		ce->Data.Forms = e->Data.Forms;
		ce->Data.Forms.EventFlag = FormEventType::ButtonClick;
		fw.PushEvent(ce);
	}
}

void GraphicButton::OnRender()
{
	sp<Image> useimage;

	if (!image && image_name != "")
	{
		image = fw.gamecore->GetImage(image_name);
		if (Size.x == 0)
		{
			Size.x = image->size.x;
		}
		if (Size.y == 0)
		{
			Size.y = image->size.y;
		}
	}
	if (imagedepressed == nullptr && imagedepressed_name != "")
	{
		imagedepressed = fw.gamecore->GetImage(imagedepressed_name);
		if (Size.x == 0)
		{
			Size.x = imagedepressed->size.x;
		}
		if (Size.y == 0)
		{
			Size.y = imagedepressed->size.y;
		}
	}
	if (imagehover == nullptr && imagehover_name != "")
	{
		imagehover = fw.gamecore->GetImage(imagehover_name);
		if (Size.x == 0)
		{
			Size.x = imagehover->size.x;
		}
		if (Size.y == 0)
		{
			Size.y = imagehover->size.y;
		}
	}

	useimage = image;
	if (mouseDepressed)
	{
		useimage = imagedepressed;
	}
	else if (mouseInside && imagehover != nullptr)
	{
		useimage = imagehover;
	}

	if (useimage != nullptr)
	{
		if (Vec2<unsigned int>{Size.x, Size.y} != useimage->size)
		{
			fw.renderer->draw(useimage, Vec2<float>{0, 0});
		}
		else
		{
			fw.renderer->drawScaled(useimage, Vec2<float>{0, 0},
			                        Vec2<float>{this->Size.x, this->Size.y});
		}
	}
}

void GraphicButton::Update() { Control::Update(); }

void GraphicButton::UnloadResources()
{
	image.reset();
	imagedepressed.reset();
	imagehover.reset();
	Control::UnloadResources();
}

sp<Image> GraphicButton::GetImage() { return image; }

void GraphicButton::SetImage(sp<Image> Image)
{
	image_name = "";
	image = Image;
}

sp<Image> GraphicButton::GetDepressedImage() { return imagedepressed; }

void GraphicButton::SetDepressedImage(sp<Image> Image)
{
	imagedepressed_name = "";
	imagedepressed = Image;
}

sp<Image> GraphicButton::GetHoverImage() { return imagehover; }

void GraphicButton::SetHoverImage(sp<Image> Image)
{
	imagehover_name = "";
	imagehover = Image;
}

Control *GraphicButton::CopyTo(Control *CopyParent)
{
	GraphicButton *copy = new GraphicButton(fw, CopyParent, this->image_name,
	                                        this->imagedepressed_name, this->imagehover_name);
	CopyControlData((Control *)copy);
	return (Control *)copy;
}

}; // namespace OpenApoc

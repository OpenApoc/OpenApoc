#include "game/ui/debugtools/imagepreview.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/textedit.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"

namespace OpenApoc
{

ImagePreview::ImagePreview() : Stage()
{
	menuform = mksp<Form>();
	menuform->Location = {0, 0};
	menuform->Size = {fw().displayGetWidth(), fw().displayGetHeight()};

	imageFilename = menuform->createChild<TextEdit>("", ui().getFont("smalfont"));
	imageFilename->Location = {5, 5};
	imageFilename->Size = {fw().displayGetWidth() - 10, 15};
	imageFilename->setText("RAW:xcom3/ufodata/isobord1.dat:640:128:xcom3/ufodata/pal_01.dat");

	imageView = menuform->createChild<Graphic>();
	updateImage();
}

ImagePreview::~ImagePreview() = default;

void ImagePreview::begin() {}

void ImagePreview::pause() {}

void ImagePreview::resume() {}

void ImagePreview::finish() {}

void ImagePreview::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION &&
	    e->forms().EventFlag == FormEventType::TextEditFinish)
	{
		updateImage();
	}
}

void ImagePreview::update() { menuform->update(); }

void ImagePreview::render() { menuform->render(); }

bool ImagePreview::isTransition() { return false; }

void ImagePreview::updateImage()
{
	auto image = fw().data->loadImage(imageFilename->getText());
	imageView->Size = image->size;
	imageView->setImage(image);
	imageView->align(HorizontalAlignment::Centre, VerticalAlignment::Centre);
}

}; // namespace OpenApoc

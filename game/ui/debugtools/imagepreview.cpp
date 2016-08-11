#include "game/ui/debugtools/imagepreview.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"

namespace OpenApoc
{

ImagePreview::ImagePreview() : Stage()
{
	menuform = mksp<Form>();
	menuform->Location = {0, 0};
	menuform->Size = {fw().Display_GetWidth(), fw().Display_GetHeight()};

	imageFilename = menuform->createChild<TextEdit>("", ui().GetFont("SMALFONT"));
	imageFilename->Location = {5, 5};
	imageFilename->Size = {fw().Display_GetWidth() - 10, 15};
	imageFilename->SetText("RAW:xcom3/ufodata/isobord1.dat:640:128:xcom3/ufodata/pal_01.dat");

	imageView = menuform->createChild<Graphic>();
	updateImage();
}

ImagePreview::~ImagePreview() {}

void ImagePreview::Begin() {}

void ImagePreview::Pause() {}

void ImagePreview::Resume() {}

void ImagePreview::Finish() {}

void ImagePreview::EventOccurred(Event *e)
{
	menuform->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION &&
	    e->Forms().EventFlag == FormEventType::TextEditFinish)
	{
		updateImage();
	}
}

void ImagePreview::Update(StageCmd *const cmd)
{
	menuform->Update();

	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void ImagePreview::Render() { menuform->Render(); }

bool ImagePreview::IsTransition() { return false; }

void ImagePreview::updateImage()
{
	auto image = fw().data->load_image(imageFilename->GetText());
	imageView->Size = image->size;
	imageView->SetImage(image);
	imageView->Location = Control::Align(HorizontalAlignment::Centre, VerticalAlignment::Centre,
	                                     imageView->GetParentSize(), imageView->Size);
}

}; // namespace OpenApoc

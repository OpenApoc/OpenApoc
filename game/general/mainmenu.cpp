
#include "mainmenu.h"
#include "../../framework/framework.h"

MainMenu::MainMenu()
{
	//emptybackground = DATA->load_bitmap( "UFODATA/TITLES.PCX" );

	//largefont = new ApocalypseFont( ApocalypseFont::LargeFont, new Palette( "UFODATA/PAL_06.DAT" ) );
	//smallfont = new ApocalypseFont( ApocalypseFont::SmallFont, new Palette( "TACDATA/TACTICAL.PAL" ) ); // "UFODATA/PAL_02.DAT" ) );
	//mousecursor = new Cursor( new Palette( "TACDATA/TACTICAL.PAL" ) );

	//testform = new Form(nullptr);
	//testform->Location.X = (FRAMEWORK->Display_GetWidth() / 2) - 322;
	//testform->Location.Y = (FRAMEWORK->Display_GetHeight() / 2) - 242;
	//testform->Size.X = 644;
	//testform->Size.Y = 484;

	//Graphic* g = new Graphic(testform, DATA->load_bitmap( "UFODATA/TITLES.PCX" ) );
	//g->Location.X = 2;
	//g->Location.Y = 2;
	//g->Size.X = testform->Size.X - 4;
	//g->Size.Y = testform->Size.Y - 4;
	////testform->Controls.push_back(g);

	//Label* l = new Label(testform, "OPEN APOCALYPSE", largefont);
	//l->BackgroundColour = al_map_rgba( 255, 255, 255, 128 );
	//l->Location.X = 2;
	//l->Location.Y = 2;
	//l->Size.X = 640;
	//l->Size.Y = 32;
	//l->TextHAlign = HorizontalAlignment::Centre;
	//l->TextVAlign = VerticalAlignment::Centre;
	////testform->Controls.push_back(l);

	//int ypos = 160;
	//Control* c = new Control(testform);
	//c->BackgroundColour = al_map_rgba( 0, 0, 0, 128 ); c->Location.X = (testform->Size.X / 2) - 146; c->Location.Y = ypos + 4; c->Size.X = 300; c->Size.Y = 28; testform->Controls.push_back(c);

	//TextButton* b = new TextButton(testform, "New Game", smallfont);
	//b->BackgroundColour = al_map_rgba( 255, 255, 255, 128 );
	//b->Location.X = (testform->Size.X / 2) - 150;
	//b->Location.Y = ypos;
	//b->Size.X = 300;
	//b->Size.Y = 28;
	////testform->Controls.push_back(b);

	//ypos += 38;

	//c = new Control(testform); c->BackgroundColour = al_map_rgba( 0, 0, 0, 128 ); c->Location.X = (testform->Size.X / 2) - 146; c->Location.Y = ypos + 4; c->Size.X = 300; c->Size.Y = 28; testform->Controls.push_back(c);

	//b = new TextButton(testform, "Load Saved Game", smallfont);
	//b->BackgroundColour = al_map_rgba( 255, 255, 255, 128 );
	//b->Location.X = (testform->Size.X / 2) - 150;
	//b->Location.Y = ypos;
	//b->Size.X = 300;
	//b->Size.Y = 28;
	////testform->Controls.push_back(b);

	//ypos += 38;

	//c = new Control(testform); c->BackgroundColour = al_map_rgba( 0, 0, 0, 128 ); c->Location.X = (testform->Size.X / 2) - 146; c->Location.Y = ypos + 4; c->Size.X = 300; c->Size.Y = 28; testform->Controls.push_back(c);

	//b = new TextButton(testform, "Options", smallfont);
	//b->BackgroundColour = al_map_rgba( 255, 255, 255, 128 );
	//b->Location.X = (testform->Size.X / 2) - 150;
	//b->Location.Y = ypos;
	//b->Size.X = 300;
	//b->Size.Y = 28;
	////testform->Controls.push_back(b);

	//ypos += 38;

	//c = new Control(testform); c->BackgroundColour = al_map_rgba( 0, 0, 0, 128 ); c->Location.X = (testform->Size.X / 2) - 146; c->Location.Y = ypos + 4; c->Size.X = 300; c->Size.Y = 28; testform->Controls.push_back(c);

	//b = new TextButton(testform, "Debugger", smallfont);
	//b->BackgroundColour = al_map_rgba( 255, 255, 255, 128 );
	//b->Location.X = (testform->Size.X / 2) - 150;
	//b->Location.Y = ypos;
	//b->Size.X = 300;
	//b->Size.Y = 28;
	////testform->Controls.push_back(b);

	//ypos += 38;

	//c = new Control(testform); c->BackgroundColour = al_map_rgba( 0, 0, 0, 128 ); c->Location.X = (testform->Size.X / 2) - 146; c->Location.Y = ypos + 4; c->Size.X = 300; c->Size.Y = 28; testform->Controls.push_back(c);

	//b = new TextButton(testform, "Quit", smallfont);
	//b->Name = "QuitButton";
	//b->BackgroundColour = al_map_rgba( 255, 255, 255, 128 );
	//b->Location.X = (testform->Size.X / 2) - 150;
	//b->Location.Y = ypos;
	//b->Size.X = 300;
	//b->Size.Y = 28;
	////testform->Controls.push_back(b);

	mousecursor = new Cursor( new Palette( "TACDATA/TACTICAL.PAL" ) );
	testform = GAMECORE->GetForm("FORM_MAINMENU");
}

MainMenu::~MainMenu()
{
	//al_destroy_bitmap( emptybackground );
	//delete largefont;
	//delete smallfont;
	delete mousecursor;
}

void MainMenu::Begin()
{
	musicplayer = new Music( 26 );
	musicplayer->Play();
}

void MainMenu::Pause()
{
}

void MainMenu::Resume()
{
}

void MainMenu::Finish()
{
	delete musicplayer;
}

void MainMenu::EventOccurred(Event *e)
{
	bool washandled = false;
	testform->EventOccured( e );
	mousecursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			FRAMEWORK->ShutdownFramework();
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		if( e->Data.Forms.RaisedBy->Name == "QuitButton" )
		{
			FRAMEWORK->ShutdownFramework();
		}
	}
}

void MainMenu::Update()
{
	testform->Update();
}

void MainMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	testform->Render();
	mousecursor->Render();
}

bool MainMenu::IsTransition()
{
	return false;
}

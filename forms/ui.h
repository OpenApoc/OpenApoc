#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class Form;
class BitmapFont;

class UI
{
  private:
	static up<UI> instance;
	std::map<UString, sp<BitmapFont>> fonts;
	std::map<UString, sp<Form>> forms;
	std::map<UString, UString> aliases;

  public:
	UI();
	~UI();

	sp<Form> getForm(UString ID);
	sp<BitmapFont> getFont(UString FontData);

	std::vector<UString> getFormIDs();

	static void unload();
	static UI &getInstance();

	void reloadFormsXml();
};

UI &ui();

}; // namespace OpenApoc

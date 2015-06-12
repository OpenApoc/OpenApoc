TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    forms/checkbox.cpp \
    forms/control.cpp \
    forms/form.cpp \
    forms/graphic.cpp \
    forms/graphicbutton.cpp \
    forms/hscrollbar.cpp \
    forms/label.cpp \
    forms/list.cpp \
    forms/textbutton.cpp \
    forms/textedit.cpp \
    forms/vscrollbar.cpp \
    framework/data.cpp \
    framework/event.cpp \
    framework/font.cpp \
    framework/framework.cpp \
    framework/image.cpp \
    framework/logger.cpp \
    framework/main.cpp \
    framework/palette.cpp \
    framework/renderer.cpp \
    framework/sound.cpp \
    framework/stagestack.cpp \
    framework/ignorecase.c \
    framework/imageloader/allegro_image.cpp \
    framework/render/allegro_renderer.cpp \
    framework/render/gl_3_0.cpp \
    framework/render/ogl_3_0_renderer.cpp \
    framework/sound/allegro_backend.cpp \
    framework/sound/null_backend.cpp \
    game/boot.cpp \
    game/gamestate.cpp \
    game/apocresources/apocfont.cpp \
    game/apocresources/apocpalette.cpp \
    game/apocresources/cursor.cpp \
    game/apocresources/music.cpp \
    game/apocresources/pck.cpp \
    game/apocresources/rawimage.cpp \
    game/apocresources/rawsound.cpp \
    game/city/building.cpp \
    game/city/buildingtile.cpp \
    game/city/city.cpp \
	game/city/cityview.cpp \
    game/city/organisation.cpp \
    game/city/vehicle.cpp \
    game/general/basescreen.cpp \
    game/general/difficultymenu.cpp \
    game/general/mainmenu.cpp \
    game/general/optionsmenu.cpp \
    game/resources/gamecore.cpp \
    game/resources/vehiclefactory.cpp \
    game/tileview/tile.cpp \
    game/tileview/tileview.cpp \
    library/configfile.cpp \
    library/memory.cpp \
    library/strings.cpp \
    game/ufopaedia/ufopaedia.cpp \
    game/debugtools/debugmenu.cpp

HEADERS += \
    forms/checkbox.h \
    forms/control.h \
    forms/form.h \
    forms/forms_enums.h \
    forms/forms.h \
    forms/graphic.h \
    forms/graphicbutton.h \
    forms/hscrollbar.h \
    forms/label.h \
    forms/list.h \
    forms/textbutton.h \
    forms/textedit.h \
    forms/vscrollbar.h \
    framework/data.h \
    framework/event.h \
    framework/font.h \
    framework/framework.h \
    framework/ignorecase.h \
    framework/image.h \
    framework/imageloader_interface.h \
    framework/includes.h \
    framework/logger.h \
    framework/musicloader_interface.h \
    framework/palette.h \
    framework/renderer_interface.h \
    framework/renderer.h \
    framework/sampleloader_interface.h \
    framework/sound_interface.h \
    framework/sound.h \
    framework/stage.h \
    framework/stagestack.h \
    framework/render/gl_3_0.hpp \
    game/boot.h \
    game/gamestate.h \
    game/apocresources/apocfont.h \
    game/apocresources/apocpalette.h \
    game/apocresources/apocresource.h \
    game/apocresources/cursor.h \
    game/apocresources/pck.h \
	game/apocresources/rawimage.h \
    game/city/building.h \
    game/city/buildingtile.h \
    game/city/city.h \
	game/city/cityview.h \
    game/city/organisation.h \
    game/city/vehicle.h \
    game/general/basescreen.h \
    game/general/difficultymenu.h \
    game/general/mainmenu.h \
    game/general/optionsmenu.h \
    game/resources/gamecore.h \
    game/resources/vehiclefactory.h \
    game/tileview/tile.h \
    game/tileview/tileview.h \
    library/angle.h \
    library/box.h \
    library/colour.h \
    library/configfile.h \
    library/line.h \
    library/maths.h \
    library/memory.h \
    library/rect.h \
    library/strings.h \
    library/vec.h \
    game/ufopaedia/ufopaedia.h \
    game/debugtools/debugmenu.h

OTHER_FILES += \
    CMakeLists.txt


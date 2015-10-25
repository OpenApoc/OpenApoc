
#pragma once

namespace OpenApoc
{

/* Work around for X11 headers defining KeyPress */
#ifdef KeyPress
#undef KeyPress
#endif

enum class FormEventType
{
	GotFocus,
	LostFocus,
	MouseEnter,
	MouseLeave,
	MouseDown,
	MouseUp,
	MouseMove,
	MouseClick,
	KeyDown,
	KeyPress,
	KeyUp,

	ButtonClick,
	CheckBoxChange,
	ScrollBarChange,
	TextChanged,
	TextEditFinish
};

enum class HorizontalAlignment
{
	Left,
	Centre,
	Right
};

enum class VerticalAlignment
{
	Top,
	Centre,
	Bottom
};

enum class FillMethod
{
	Stretch,
	Fit,
	Tile
};

}; // namespace OpenApoc

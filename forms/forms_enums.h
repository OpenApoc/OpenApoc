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
	TextInput,

	ButtonClick,
	CheckBoxChange,
	CheckBoxSelected,
	CheckBoxDeSelected,
	TriStateBoxChange,
	TriStateBoxState1Selected,
	TriStateBoxState2Selected,
	TriStateBoxState3Selected,
	ScrollBarChange,
	TextChanged,
	TextEditFinish,
	TextEditCancel,
	ListBoxChangeHover,
	ListBoxChangeSelected
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

enum class Orientation
{
	Vertical,
	Horizontal
};

}; // namespace OpenApoc

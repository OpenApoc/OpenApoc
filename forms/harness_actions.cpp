#include "forms/harness_actions.h"
#include "forms/checkbox.h"
#include "forms/control.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/textbutton.h"
#include "forms/textedit.h"
#include "framework/framework.h"
#include "framework/harness.h"
#include "library/strings.h"
#include "library/strings_format.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <vector>

namespace OpenApoc
{
namespace
{

uint64_t formsFrame = 0;
std::vector<wp<Form>> visibleForms;

uint64_t currentFrame()
{
	if (auto *instance = Framework::tryGetInstance())
	{
		return instance->getFrameNumber();
	}
	return 0;
}

UString controlTypeName(Control *control)
{
	if (dynamic_cast<TextButton *>(control))
	{
		return "TextButton";
	}
	if (dynamic_cast<GraphicButton *>(control))
	{
		return "GraphicButton";
	}
	if (dynamic_cast<RadioButton *>(control))
	{
		return "RadioButton";
	}
	if (dynamic_cast<CheckBox *>(control))
	{
		return "CheckBox";
	}
	if (dynamic_cast<ScrollBar *>(control))
	{
		return "ScrollBar";
	}
	if (dynamic_cast<ListBox *>(control))
	{
		return "ListBox";
	}
	if (dynamic_cast<TextEdit *>(control))
	{
		return "TextEdit";
	}
	if (dynamic_cast<Label *>(control))
	{
		return "Label";
	}
	if (dynamic_cast<Form *>(control))
	{
		return "Form";
	}
	return "Control";
}

std::vector<sp<Form>> liveForms()
{
	std::vector<sp<Form>> out;
	std::set<Form *> seen;
	for (auto &weak : visibleForms)
	{
		auto form = weak.lock();
		if (!form || !seen.insert(form.get()).second)
		{
			continue;
		}
		out.push_back(form);
	}
	return out;
}

sp<Control> findNamedControl(const UString &id)
{
	for (auto &form : liveForms())
	{
		if (form->Name == id)
		{
			return form;
		}
		if (auto found = form->findControl(id))
		{
			return found;
		}
	}
	return nullptr;
}

void collectNamed(const sp<Control> &control, std::vector<UString> &out)
{
	if (!control)
	{
		return;
	}
	if (!control->Name.empty() && control->Name != "Control")
	{
		out.push_back(format("{0}:{1}:visible={2}:enabled={3}", control->Name,
		                     controlTypeName(control.get()), control->isVisible() ? 1 : 0,
		                     control->Enabled ? 1 : 0));
	}
	for (auto &child : control->Controls)
	{
		collectNamed(child, out);
	}
}

bool parseBoolValue(const UString &value, bool &out)
{
	const auto lowered = to_lower(value);
	if (lowered == "1" || lowered == "true" || lowered == "on" || lowered == "yes")
	{
		out = true;
		return true;
	}
	if (lowered == "0" || lowered == "false" || lowered == "off" || lowered == "no")
	{
		out = false;
		return true;
	}
	return false;
}

bool parseIntValue(const UString &value, int &out)
{
	if (value.empty())
	{
		return false;
	}
	char *end = nullptr;
	const long parsed = std::strtol(value.c_str(), &end, 10);
	if (!end || *end != '\0')
	{
		return false;
	}
	out = static_cast<int>(parsed);
	return true;
}

// The gap the named-action layer leaves is runtime widgets with no ids: purchase rows, lab
// entries, agent rows. They are real controls in the tree, they simply were never given a Name,
// so the only handle on them is their position in their parent. These two helpers turn that
// position into an address, which keeps the driver operating the actual UI rather than reaching
// past it into GameState.

// Nth child of a control, skipping nothing -- index is the raw child order the engine built.
sp<Control> itemAt(const sp<Control> &parent, int index)
{
	if (!parent || index < 0 || index >= static_cast<int>(parent->Controls.size()))
	{
		return nullptr;
	}
	return parent->Controls[static_cast<size_t>(index)];
}

// First descendant that "set" knows how to drive. A purchase row is a bare Control whose
// quantity is an unnamed ScrollBar child (transactioncontrol.cpp:702), so setting the row means
// setting that scrollbar.
sp<Control> firstSettableDescendant(const sp<Control> &root, int depth = 0)
{
	if (!root || depth > 8)
	{
		return nullptr;
	}
	for (auto &child : root->Controls)
	{
		if (!child)
		{
			continue;
		}
		if (dynamic_cast<ScrollBar *>(child.get()) || dynamic_cast<CheckBox *>(child.get()) ||
		    dynamic_cast<TextEdit *>(child.get()))
		{
			return child;
		}
	}
	for (auto &child : root->Controls)
	{
		if (auto found = firstSettableDescendant(child, depth + 1))
		{
			return found;
		}
	}
	return nullptr;
}

// First label text anywhere under a control. Runtime list rows carry their identity only as a
// child Label -- a purchase row's item name, a lab's facility name -- so without this a driver
// can address rows by position but has no idea which row is which, and ends up buying whatever
// happens to be in slot 3.
UString firstLabelText(const sp<Control> &root, int depth = 0)
{
	if (!root || depth > 8)
	{
		return "";
	}
	for (auto &child : root->Controls)
	{
		if (auto *lbl = dynamic_cast<Label *>(child.get()))
		{
			const auto t = lbl->getText();
			if (!t.empty())
			{
				return t;
			}
		}
	}
	for (auto &child : root->Controls)
	{
		const auto found = firstLabelText(child, depth + 1);
		if (!found.empty())
		{
			return found;
		}
	}
	return "";
}

UString applyToControl(const sp<Control> &control, const UString &label, const UString &op,
                       const UString &value);

UString applyControl(const UString &id, const UString &op, const UString &value)
{
	auto control = findNamedControl(id);
	if (!control)
	{
		return format("ERR unknown control \"{0}\"", id);
	}
	return applyToControl(control, id, op, value);
}

UString applyToControl(const sp<Control> &control, const UString &id, const UString &op,
                       const UString &value)
{
	if (op == "click" || op.empty())
	{
		if (!control->click())
		{
			return format("ERR control \"{0}\" not clickable (visible={1} enabled={2})", id,
			              control->isVisible() ? 1 : 0, control->Enabled ? 1 : 0);
		}
		return format("OK clicked {0}", id);
	}
	if (op == "toggle")
	{
		if (auto *box = dynamic_cast<CheckBox *>(control.get()))
		{
			box->setChecked(!box->isChecked());
			return format("OK {0} checked={1}", id, box->isChecked() ? 1 : 0);
		}
		if (!control->click())
		{
			return format("ERR control \"{0}\" cannot toggle", id);
		}
		return format("OK toggled {0}", id);
	}
	// Reading a widget's current value matters as much as writing it: a transaction row's
	// quantity is a balance, so setting it *below* what the base already holds sells the
	// difference. A driver that cannot read the current value ends up selling the squad's
	// weapons while believing it is buying them.
	if (op == "get")
	{
		if (auto *box = dynamic_cast<CheckBox *>(control.get()))
		{
			return format("OK {0} checked={1}", id, box->isChecked() ? 1 : 0);
		}
		if (auto *bar = dynamic_cast<ScrollBar *>(control.get()))
		{
			return format("OK {0} value={1} min={2} max={3}", id, bar->getValue(),
			              bar->getMinimum(), bar->getMaximum());
		}
		if (auto *list = dynamic_cast<ListBox *>(control.get()))
		{
			return format("OK {0} items={1}", id, static_cast<int>(list->Controls.size()));
		}
		if (auto *lbl = dynamic_cast<Label *>(control.get()))
		{
			auto t = lbl->getText();
			std::replace(t.begin(), t.end(), ' ', '_');
			return format("OK {0} text={1}", id, t.empty() ? UString("-") : t);
		}
		if (auto inner = firstSettableDescendant(control))
		{
			return applyToControl(inner, format("{0}/inner", id), "get", "");
		}
		return format("ERR control \"{0}\" ({1}) has no readable value", id,
		              controlTypeName(control.get()));
	}
	if (op == "set")
	{
		if (auto *box = dynamic_cast<CheckBox *>(control.get()))
		{
			bool checked = false;
			if (!parseBoolValue(value, checked))
			{
				return format("ERR set {0} needs true/false", id);
			}
			box->setChecked(checked);
			return format("OK {0} checked={1}", id, box->isChecked() ? 1 : 0);
		}
		if (auto *bar = dynamic_cast<ScrollBar *>(control.get()))
		{
			int parsed = 0;
			if (!parseIntValue(value, parsed))
			{
				return format("ERR set {0} needs an integer", id);
			}
			if (!bar->setValue(parsed))
			{
				return format("ERR set {0} rejected value {1} (min={2} max={3})", id, parsed,
				              bar->getMinimum(), bar->getMaximum());
			}
			return format("OK {0} value={1}", id, bar->getValue());
		}
		if (auto *edit = dynamic_cast<TextEdit *>(control.get()))
		{
			edit->setText(value);
			return format("OK {0} text set", id);
		}
		if (auto *list = dynamic_cast<ListBox *>(control.get()))
		{
			int index = 0;
			if (!parseIntValue(value, index) || index < 0 ||
			    index >= static_cast<int>(list->Controls.size()))
			{
				return format("ERR set {0} needs an item index 0..{1}", id,
				              static_cast<int>(list->Controls.size()) - 1);
			}
			// Neither Control::click() nor setSelected() actually selects a list item: the
			// first raises MouseClick, which ListBox ignores for selection, and the second
			// raises nothing at all. Both would report success while the screen behind the list
			// never changed.
			if (!list->selectItemByIndex(static_cast<size_t>(index)))
			{
				return format("ERR set {0} could not select item {1}", id, index);
			}
			return format("OK {0} selected {1}", id, index);
		}
		// Rows in a runtime-built list are plain Controls whose editable part is an unnamed
		// child -- the purchase quantity is a ScrollBar inside the row. Reach it rather than
		// refusing.
		if (auto inner = firstSettableDescendant(control))
		{
			return applyToControl(inner, format("{0}/inner", id), "set", value);
		}
		return format("ERR control \"{0}\" ({1}) does not support set", id,
		              controlTypeName(control.get()));
	}
	return format("ERR unknown control op \"{0}\"", op);
}

UString handleFormAction(const UString &verb, const std::vector<UString> &args)
{
	const auto action = to_lower(verb);
	if (action == "help")
	{
		return "OK verbs=help,controls,control,click,set,get,toggle,item "
		       "usage=CONTROL <id> [click|toggle|set <value>|item <N> [op] [value]] "
		       "CONTROLS [<id>] lists children of <id> by position";
	}
	if (action == "controls")
	{
		// With an id, enumerate that control's immediate children by position, including the
		// unnamed ones. A driver cannot address what it cannot see, and the rows that matter --
		// purchase lines, lab entries -- have no names to list.
		if (!args.empty())
		{
			auto parent = findNamedControl(args[0]);
			if (!parent)
			{
				return format("ERR unknown control \"{0}\"", args[0]);
			}
			UString reply = format("OK parent={0} items={1}", args[0], parent->Controls.size());
			for (size_t i = 0; i < parent->Controls.size(); i++)
			{
				const auto &child = parent->Controls[i];
				if (!child)
				{
					continue;
				}
				const auto inner = firstSettableDescendant(child);
				auto text = firstLabelText(child);
				std::replace(text.begin(), text.end(), ' ', '_');
				reply += format(" {0}:{1}:{2}:visible={3}:settable={4}:text={5}", i,
				                child->Name.empty() ? UString("-") : child->Name,
				                controlTypeName(child.get()), child->isVisible() ? 1 : 0,
				                inner ? controlTypeName(inner.get()) : UString("-"),
				                text.empty() ? UString("-") : text);
			}
			return reply;
		}
		std::vector<UString> names;
		for (auto &form : liveForms())
		{
			collectNamed(form, names);
		}
		UString reply = format("OK count={0}", names.size());
		for (auto &name : names)
		{
			reply += " ";
			reply += name;
		}
		return reply;
	}
	if (action == "click")
	{
		if (args.empty())
		{
			return "ERR click needs a control id";
		}
		return applyControl(args[0], "click", "");
	}
	if (action == "toggle")
	{
		if (args.empty())
		{
			return "ERR toggle needs a control id";
		}
		return applyControl(args[0], "toggle", "");
	}
	if (action == "set")
	{
		if (args.size() < 2)
		{
			return "ERR set needs a control id and a value";
		}
		UString value;
		for (size_t i = 1; i < args.size(); i++)
		{
			if (i > 1)
			{
				value += " ";
			}
			value += args[i];
		}
		return applyControl(args[0], "set", value);
	}
	if (action == "control")
	{
		if (args.empty())
		{
			return "ERR CONTROL needs a control id";
		}
		auto control = findNamedControl(args[0]);
		if (!control)
		{
			return format("ERR unknown control \"{0}\"", args[0]);
		}
		UString label = args[0];
		size_t i = 1;
		// Walk any number of "item <N>" hops before the terminal operation. One level covers a
		// plain list row; some widgets need more. MapSelector's map rows are the case that
		// forced this: the row itself is an inert Control with no click handler at all, and the
		// button that actually picks the map (calling Skirmish::setLocation) is a *second-level*
		// child of that row -- not the row itself, and not something ListBox selection reaches
		// either, since MapSelector never listens for ListBoxChangeSelected. Only
		// `item <N> item 1 click` reaches it without pixel geometry.
		while (i + 1 < args.size() && to_lower(args[i]) == "item")
		{
			int index = 0;
			if (!parseIntValue(args[i + 1], index))
			{
				return format("ERR item index \"{0}\" is not a number", args[i + 1]);
			}
			auto next = itemAt(control, index);
			if (!next)
			{
				return format("ERR {0} has no item {1} (items={2})", label, index,
				              static_cast<int>(control->Controls.size()));
			}
			control = next;
			label = format("{0}[{1}]", label, index);
			i += 2;
		}
		UString op = i < args.size() ? to_lower(args[i]) : UString("click");
		UString value;
		if (op == "set")
		{
			for (size_t j = i + 1; j < args.size(); j++)
			{
				if (j > i + 1)
				{
					value += " ";
				}
				value += args[j];
			}
		}
		return applyToControl(control, label, op, value);
	}
	return "";
}

} // namespace

void notifyVisibleForm(const sp<Form> &form)
{
	if (!form)
	{
		return;
	}
	installFormsHarnessActions();
	const auto frame = currentFrame();
	if (frame != formsFrame)
	{
		visibleForms.clear();
		formsFrame = frame;
	}
	visibleForms.emplace_back(form);
}

void installFormsHarnessActions()
{
	static bool installed = false;
	if (installed)
	{
		return;
	}
	installed = true;
	auto previous = getHarnessActionHandler();
	setHarnessActionHandler(
	    [previous](const UString &verb, const std::vector<UString> &args) -> UString
	    {
		    auto reply = handleFormAction(verb, args);
		    if (!reply.empty())
		    {
			    return reply;
		    }
		    if (previous)
		    {
			    return previous(verb, args);
		    }
		    return "";
	    });
}

} // namespace OpenApoc

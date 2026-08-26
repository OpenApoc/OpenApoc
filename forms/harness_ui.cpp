// Live UI introspection for the test harness.
//
// The harness used to locate controls by parsing data/forms/**.form and recomputing the layout
// itself. That only worked while the viewport was a fixed size and the UI was unscaled. With a
// resizable display and a UI scale factor, the only trustworthy source of a control's position is
// the control itself, after the engine has resolved it.
//
// framework/ cannot name Control (OpenApoc_Forms links OpenApoc_Framework, never the reverse), so
// this lives here and installs itself into the framework's UI hook.

#include "forms/control.h"
#include "forms/form.h"
#include "forms/harness_ui.h"
#include "framework/harness.h"
#include "library/strings_format.h"
#include <algorithm>
#include <set>

namespace OpenApoc
{
namespace
{

// One control per record: id, class, resolved screen rect, visibility. Tab separated inside a
// record, semicolon between records, so the whole dump stays on the harness's single reply line.
void dumpControl(const sp<Control> &c, const UString &filter, UString &out, int &count, int depth,
                 std::set<UString> &seen)
{
	if (!c || depth > 12)
	{
		return;
	}
	const auto name = c->Name;
	const bool named = !name.empty() && name != "Control";
	if (named && (filter.empty() || to_lower(name).find(to_lower(filter)) != UString::npos))
	{
		const auto pos = c->getLocationInUi();
		// ui().getForm() caches a template and hands out a copy, so both are live and identical.
		// Collapse exact duplicates rather than reporting every control twice.
		const auto key = format("{0},{1},{2},{3},{4}", name, pos.x, pos.y, c->Size.x, c->Size.y);
		if (seen.insert(key).second)
		{
			if (count++ > 0)
			{
				out += ";";
			}
			out += format("{0},{1}", key, c->isVisible() ? 1 : 0);
		}
	}
	for (const auto &child : c->Controls)
	{
		dumpControl(child, filter, out, count, depth + 1, seen);
	}
}

UString dumpLiveUI(const UString &filter)
{
	UString out;
	int count = 0;
	std::set<UString> seen;
	for (auto *form : Form::liveForms())
	{
		if (!form || !form->isVisible())
		{
			continue;
		}
		// Walk from the form itself so the root's own rect is reported too.
		for (const auto &child : form->Controls)
		{
			dumpControl(child, filter, out, count, 1, seen);
		}
		const auto pos = form->getLocationInUi();
		if (filter.empty() || to_lower(form->Name).find(to_lower(filter)) != UString::npos)
		{
			const auto key = format("{0},{1},{2},{3},{4}", form->Name, pos.x, pos.y, form->Size.x,
			                        form->Size.y);
			if (seen.insert(key).second)
			{
				if (count++ > 0)
				{
					out += ";";
				}
				out += format("{0},1", key);
			}
		}
	}
	return format("count={0} at={1}", count, out.empty() ? UString("-") : out);
}

} // namespace

void registerFormsHarnessUI() { setHarnessUIHandler(dumpLiveUI); }

} // namespace OpenApoc

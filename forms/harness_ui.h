#pragma once

namespace OpenApoc
{

// Installs the live-UI dump behind the harness UI command. Call once at startup; the forms layer
// answers it, because framework/ cannot name Control.
void registerFormsHarnessUI();

} // namespace OpenApoc

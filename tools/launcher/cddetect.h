#pragma once

#include <QString>
#include <QStringList>

namespace OpenApoc::CDDetect
{

// Lightweight plausibility check (existence + extension/marker only, no PhysFS mounting).
// Used both to decide whether the current CD path needs (re)detection and to validate
// scan-produced candidates.
bool isPlausibleCDPath(const QString &path);

// Scans known locations (local ./data/cd.iso, Steam appid 7660, GOG registry entry,
// mounted/extracted volumes) for a plausible X-COM: Apocalypse data source.
QStringList detectCandidates();

} // namespace OpenApoc::CDDetect

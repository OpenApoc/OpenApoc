#pragma once
#include <cstdint>

namespace OpenApoc
{

#define ORGANISATION_NAME_STRTAB_OFFSET_START 1355537
#define ORGANISATION_NAME_STRTAB_OFFSET_END 1355822

// FIXME: The xcom starting funds u32 is clearly at 0x0141470 in my ufo2p.exe, and the initial
// income u32 at 0x0141474, but everything beyond that doesn't seem to map well to the other
// organisations
//
// Income may then also be modified by a per-tile value based on owned buildings & their 'types'?
#define ORGANISATION_FUNDING_DATA_OFFSET_START (0x141468)

} // namespace OpenApoc

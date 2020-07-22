//
// Created by sf on 4/15/16.
//
#pragma once

#include "library/strings.h"
#include <physfs.h>

namespace OpenApoc
{
void parseCueFile(UString fileName);
PHYSFS_Archiver *getCueArchiver();
} // namespace OpenApoc

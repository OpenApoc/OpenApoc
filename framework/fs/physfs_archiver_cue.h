//
// Created by sf on 4/15/16.
//
#pragma once

#include <physfs.h>
#include "library/strings.h"

namespace OpenApoc
{
    void parseCueFile(UString fileName);
    PHYSFS_Archiver *getCueArchiver();
}

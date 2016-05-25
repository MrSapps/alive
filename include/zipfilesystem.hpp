#pragma once

#include "resourcemapper.hpp"
#include "unzip.h"
#include "ioapi.h"

// Actually "ZIP64" file system, which removes 65k file limit and 4GB zip file size limit
class ZipFileSystem
{
public:
    ZipFileSystem()
    {
        zlib_filefunc_def filefunc32 = { 0 };


        unzOpen2("__notused__", &filefunc32);
    }
};

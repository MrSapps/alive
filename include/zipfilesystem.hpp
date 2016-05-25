#pragma once

#include "resourcemapper.hpp"
#include "unzip.h"
#include "ioapi.h"
#include "iowin32.h"

// Actually "ZIP64" file system, which removes 65k file limit and 4GB zip file size limit
class ZipFileSystem
{
public:
    ZipFileSystem()
    {
        zlib_filefunc64_def filefunc32 = { };

        //fill_win32_filefunc64(&filefunc32);
        fill_fopen64_filefunc(&filefunc32);

        unzFile  f = unzOpen2_64("__notused__", &filefunc32);
        if (f)
        {
            unz_file_info64 fileInfo = {};
            unzGetCurrentFileInfo64(f, &fileInfo, "test.txt", 0, 0, 0, 0, 0);

            char buffer[10];
            unzReadCurrentFile(f, buffer, 10);

            unzClose(f);
        }
    }
};

#pragma once

extern "C"
{
    // Hack to make SDL2 built with MSVC 2013 link with MSVC 2015
#if _MSC_VER == 1900
    void *__iob_func()
    {
        static FILE f[] = { *stdin, *stdout, *stderr };
        return f;
    }
#endif
}

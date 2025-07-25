#ifndef VERSION_H
#define VERSION_H

#define VER_STRINGIFY(d)            #d
#define TO_STR(v)                   VER_STRINGIFY(v)

#define VER_STR_LONG(mj,mn,b,r)     VER_STRINGIFY(mj) "." VER_STRINGIFY(mn) "." VER_STRINGIFY(b) "." VER_STRINGIFY(r) "\0"
#define VER_STR_SHORT(mj,mn)        VER_STRINGIFY(mj) "." VER_STRINGIFY(mn) "\0"

#define VER_MAJOR                   1
#define VER_MINOR                   0
#define VER_BUILD                   0
#define VER_REVISION                0

#define VER_NUMBER                  VER_MAJOR,VER_MINOR,VER_BUILD,VER_REVISION
#define VER_STRING                  VER_STR_LONG(VER_MAJOR,VER_MINOR,VER_BUILD,VER_REVISION)
#define VER_STRING_SHORT            VER_STR_SHORT(VER_MAJOR,VER_MINOR)

#define VER_FILEVERSION             VER_NUMBER
#define VER_FILEVERSION_STR         VER_STRING

#define VER_PRODUCTVERSION          VER_FILEVERSION
#define VER_PRODUCTVERSION_STR      VER_STRING_SHORT

#define VER_LANG_AND_CHARSET_STR    "040904E4"
#define VER_LANG_ID                 0x0409
#define VER_CHARSET_ID              1252

#define COPYRIGHT_YEAR              2025

#define VER_FILEDESCRIPTION_STR     "Svg2EmfPlus\0"
#define VER_PRODUCTNAME_STR         "Svg2EmfPlus\0"
#define VER_COMPANYNAME_STR         "Open Source Community\0"
#define VER_LEGALCOPYRIGHT_STR      "Open Source Community 2025\0"
#define VER_COMPANYDOMAIN_STR       "localhost\0"

#define VER_INTERNALNAME_STR        "Svg2EmfPlus\0"
#define VER_LEGALTRADEMARKS1_STR    "All rights reserved\0"
#define VER_LEGALTRADEMARKS2_STR    VER_LEGALTRADEMARKS1_STR
#define VER_ORIGINALFILENAME_STR    "svg2emfplus.exe\0"

#endif // VERSION_H

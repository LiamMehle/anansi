// #include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"

#define ZLIB_VERSION "1.3.1"
#include "zlib.h"

int main(void) {
    char buffer_in [256] = {"Conan is a MIT-licensed, Open Source package manager for C and C++ development "
                            "for C and C++ development, allowing development teams to easily and efficiently "
                            "manage their packages and dependencies across platforms and build systems."};
    char buffer_out [512] = {0};

    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    int err = deflateInit(&defstream, Z_BEST_COMPRESSION);
    if (err != Z_OK) {
        printf("init error %i\n", err);
        return 0;
    }

    defstream.data_type = Z_TEXT;
    defstream.avail_in = (uInt) strlen(buffer_in);
    defstream.next_in = (Bytef *) buffer_in;
    defstream.avail_out = (uInt) sizeof(buffer_out);
    defstream.next_out = (Bytef *) buffer_out;
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    printf("ZLIB VERSION: %s\n", zlibVersion());
    if (defstream.msg)
        printf("Error: %s\n", defstream.msg);
    else {
        printf("Uncompressed size is: %llu\n", strlen(buffer_in));
        printf("Compressed size is: %llu\n", strlen(buffer_out));
    }

    return 0;
}
#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(void)
{
    Manifest* manifest = parse_manifest("manifest.json");
    if (manifest == NULL)
    {
        perror("error");
        return 1;
    }

    const int result = extraction("hello-1.0.0.tar.gz", "/tmp/xpkg_test");
    if (result != 0)
    {
        free_manifest(manifest);
        return 1;
    }

    write_package_record(manifest);

    copy_files(
        (const char**)manifest->files,
        manifest->file_count,
        "/tmp/xpkg_install",
        manifest->name,
        "/tmp/xpkg_test/hello-1.0.0"
    );

    free_manifest(manifest);
    return 0;
}

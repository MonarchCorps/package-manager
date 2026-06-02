#include "manager.h"
#include <stdio.h>
#include <string.h>

int main(const int argc, const char** argv)
{
    if (
        (argc == 2 && strcmp(argv[1], "--help") == 0)
        || argc > 3
        || argc == 1
    )
    {
        printf("Usage: xpkg <command> [args]\n");
        printf("Commands:\n");
        printf("  install <package.tar.gz>  Install a package\n");
        printf("  list                      List installed packages\n");
        printf("  remove <package-name>     Remove a package\n");
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "install") == 0)
        install(argv[2]);
    else if (argc == 2 && strcmp(argv[1], "list") == 0)
        list();
    else if (argc == 3 && strcmp(argv[1], "remove") == 0)
        remove_package(argv[2]);
    else
    {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
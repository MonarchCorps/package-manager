//
// Created by David Okocha on 01/06/2026.
//

#ifndef PACKAGE_MANAGER_MANAGER_H
#define PACKAGE_MANAGER_MANAGER_H

#include <unistd.h>

typedef struct
{
    char* name;
    char* version;
    char** files;
    size_t file_count;
} Manifest;

Manifest* parse_manifest(const char* filename);
char* extract_key(const char* buffer, const char* field_name);
char** parse_files(size_t* file_count, const char* buffer);
int extraction(char* archive_path, char* destination_directory);
int write_package_record(const Manifest* manifest);
void free_manifest(Manifest* manifest);
int copy_file(const char* source_path, const char* destination_path);
int copy_files(
    const char** files,
    size_t file_count,
    const char* install_dir,
    const char* package_name,
    const char* extracted_dir
);
void round_cleanup(
    Manifest* manifest,
    char* buffer,
    char* name,
    char* version,
    char** files
);


#endif //PACKAGE_MANAGER_MANAGER_H

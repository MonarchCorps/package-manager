//
// Created by David Okocha on 01/06/2026.
//

#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>

Manifest* parse_manifest(const char* filename)
{
    FILE* pF = fopen(filename, "r");
    if (pF == NULL)
    {
        perror("Failed to open file");
        return NULL;
    }

    fseek(pF, 0, SEEK_END);
    const long file_size = ftell(pF);
    fseek(pF, 0, SEEK_SET);

    char* buffer = malloc(file_size + 1); // null terminator included
    if (buffer == NULL)
    {
        perror("Out of memory");
        fclose(pF);
        return NULL;
    }

    fread(buffer, file_size, 1, pF);
    buffer[file_size] = 0;

    fclose(pF);

    Manifest* newManifest = malloc(sizeof(Manifest));
    if (newManifest == NULL)
    {
        perror("Out of memory");
        free(buffer);
        return NULL;
    }

    char* name = extract_key(buffer, "name");
    char* version = extract_key(buffer, "version");
    char** files = parse_files(&newManifest->file_count, buffer);

    if (name == NULL || version == NULL || files == NULL)
    {
        round_cleanup(newManifest, buffer, name, version, files);
        return NULL;
    }

    newManifest->name = name;
    newManifest->version = version;
    newManifest->files = files;

    free(buffer);

    return newManifest;
}

char* extract_key(const char* buffer, const char* field_name)
{
    const char* keyP = strstr(buffer, field_name);

    if (keyP == NULL)
        return NULL;

    const char* colon = strchr(keyP, ':');
    if (colon == NULL) return NULL;

    const char* startKeyP = colon + 3;
    const char* endKeyP = strchr(startKeyP, '"');

    if (startKeyP == NULL || endKeyP == NULL)
        return NULL;

    char* key = malloc(endKeyP - startKeyP + 1);
    memcpy(key, startKeyP, endKeyP - startKeyP);
    key[endKeyP - startKeyP] = '\0';

    return key;
}

char** parse_files(size_t* file_count, const char* buffer)
{
    const char* filesP = strstr(buffer, "files");

    if (filesP == NULL)
        return NULL;

    const char* colon = strchr(filesP, ':');
    if (colon == NULL) return NULL;

    const char* startFilesP = colon + 3;

    size_t files_capacity = 4;

    char** files = malloc(files_capacity * sizeof(char*));

    if (files == NULL)
    {
        perror("Out of memory");
        return NULL;
    }

    while (1)
    {
        const char* loopColon = strchr(startFilesP, '"');
        if (loopColon == NULL) return NULL;

        const char* firstQuote = loopColon + 1;

        const char* lastQuote = strchr(firstQuote, '"');

        char* key = malloc(lastQuote - firstQuote + 1);
        memcpy(key, firstQuote, lastQuote - firstQuote);
        key[lastQuote - firstQuote] = '\0';

        if (*file_count >= files_capacity)
        {
            files_capacity *= 2;
            char** temp = realloc(files, files_capacity * sizeof(char*));
            if (temp == NULL)
            {
                perror("Out of memory");
                for (size_t i = 0; i < *file_count; i++)
                {
                    free(files[i]);
                }
                free(files);
                return NULL;
            }
            files = temp;
        }

        files[(*file_count)++] = key;

        const char* next_quote = strchr(lastQuote + 1, '"');
        const char* closing_bracket = strchr(lastQuote + 1, ']');
        if (closing_bracket != NULL && (next_quote == NULL || closing_bracket < next_quote))
        {
            break;
        }

        startFilesP = lastQuote + 1;
    }

    return files;
}

int extraction(char* archive_path, char* destination_directory)
{
    if (access(archive_path, F_OK) != 0) return -1;

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "tar -xzf %s -C %s", archive_path, destination_directory);

    const int mkdir_result = mkdir(destination_directory, 0755);
    if (mkdir_result == -1 && errno != EEXIST) return -1;

    FILE* pF = popen(buffer, "r");
    if (pF == NULL)
    {
        return -1;
    }
    const int result = pclose(pF);
    if (result != 0) return -1;
    return 0;
}

int write_package_record(const Manifest* manifest)
{
    if (manifest == NULL)
    {
        perror("manifest is null");
        return -1;
    }

    FILE* pF = fopen("xpkg.db", "a");
    if (pF == NULL)
    {
        perror("failed to open database file");
        return -1;
    }

    size_t flat_ile_capacity = 20;
    char* flat_file = malloc(flat_ile_capacity * sizeof(char));

    if (flat_file == NULL)
    {
        perror("out of memory");
        fclose(pF);
        return -1;
    }
    flat_file[0] = '\0';

    for (size_t i = 0; i < manifest->file_count; i++)
    {
        if (strlen(flat_file) + strlen(manifest->files[i]) + 1 > flat_ile_capacity)
        {
            flat_ile_capacity *= 2;
            char* temp = realloc(flat_file, flat_ile_capacity * sizeof(char));
            if (temp == NULL)
            {
                perror("out of memory");
                free(flat_file);
                fclose(pF);
                return -1;
            }
            flat_file = temp;
        }

        strcat(flat_file, manifest->files[i]);
        if (i < manifest->file_count - 1)
            strcat(flat_file, ",");
    }

    fprintf(pF, "%s|%s|%s\n", manifest->name, manifest->version, flat_file);

    free(flat_file);
    fclose(pF);

    return 0;
}

int copy_file(const char* source_path, const char* destination_path)
{
    FILE* pF = fopen(source_path, "rb");
    if (pF == NULL)
    {
        perror("Failed to open source file");
        return -1;
    }

    FILE* pDestF = fopen(destination_path, "wb");
    if (pDestF == NULL)
    {
        perror("Failed to open destination file");
        fclose(pF);
        return -1;
    }

    fseek(pF, 0, SEEK_END);
    const long file_size = ftell(pF);
    fseek(pF, 0, SEEK_SET);

    char* buffer = malloc(file_size + 1); // null terminator included
    if (buffer == NULL)
    {
        perror("Out of memory");
        fclose(pF);
        fclose(pDestF);
        return -1;
    }

    const size_t bytes_read = fread(buffer, 1, file_size, pF);
    buffer[file_size] = 0;

    fwrite(buffer, bytes_read, 1, pDestF);

    free(buffer);
    fclose(pF);
    fclose(pDestF);

    return 0;
}

int copy_files(
    const char** files,
    const size_t file_count,
    const char* install_dir,
    const char* package_name,
    const char* extracted_dir
)
{
    if (files == NULL)
    {
        perror("files cannot be NULL");
        return -1;
    }

    for (size_t i = 0; i < file_count; i++)
    {
        char path_copy[1024];
        snprintf(path_copy, sizeof(path_copy), "%s", files[i]);
        char* dir_path = dirname(path_copy);

        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s/%s/%s", install_dir, package_name, dir_path);
        system(cmd);

        char dest_path[1024];
        snprintf(dest_path, sizeof(dest_path), "%s/%s/%s", install_dir, package_name, files[i]);

        char source_path[1024];
        snprintf(source_path, sizeof(source_path), "%s/%s", extracted_dir, files[i]);

        char error_message[1024] = "";

        const int result = copy_file(source_path, dest_path);
        if (result == -1)
        {
            snprintf(error_message, sizeof(error_message), "Could not copy %s to destination directory", files[i]);
            perror(error_message);
            return -1;
        }
    }

    return 0;
}

void free_manifest(Manifest* manifest)
{
    free(manifest->name);
    free(manifest->version);
    for (size_t i = 0; i < manifest->file_count; i++)
        free(manifest->files[i]);
    free(manifest->files);
    free(manifest);
}

void round_cleanup(
    Manifest* manifest,
    char* buffer,
    char* name,
    char* version,
    char** files
)
{
    free(name);
    free(version);
    if (files != NULL)
    {
        for (size_t i = 0; i < manifest->file_count; i++)
        {
            free(files[i]);
        }
        free(files);
    }
    free(buffer);
    free(manifest);
}

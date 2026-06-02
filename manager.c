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
#include <dirent.h>

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

    const char* startKeyP = colon + 2;
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

    const char* startFilesP = colon + 1;

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
        fprintf(stderr, "manifest is null\n");
        return -1;
    }

    FILE* pF = fopen("xpkg.db", "a");
    if (pF == NULL)
    {
        perror("failed to open database file");
        return -1;
    }

    fprintf(pF, "%s|%s|", manifest->name, manifest->version);

    for (size_t i = 0; i < manifest->file_count; i++)
    {
        fprintf(pF, "%s", manifest->files[i]);
        if (i < manifest->file_count - 1)
            fprintf(pF, ",");
    }

    fprintf(pF, "\n");
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

int install(const char* archive_path)
{
    const char* extract_dir = "/tmp/xpkg_extract";
    const int extract_result = extraction((char*)archive_path, (char*)extract_dir);
    if (extract_result != 0)
    {
        fprintf(stderr, "Failed to extract %s\n", archive_path);
        return -1;
    }

    char* pkg_dir = find_extracted_dir(extract_dir);
    if (pkg_dir == NULL)
    {
        fprintf(stderr, "Failed to find extracted package directory\n");
        return -1;
    }

    char manifest_path[1024];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", pkg_dir);

    Manifest* manifest = parse_manifest(manifest_path);
    if (manifest == NULL)
    {
        free(pkg_dir);
        fprintf(stderr, "Failed to parse manifest\n");
        return -1;
    }

    const int copy_result = copy_files(
        (const char**)manifest->files,
        manifest->file_count,
        "/tmp/xpkg_install",
        manifest->name,
        pkg_dir
    );

    free(pkg_dir);

    if (copy_result != 0)
    {
        free_manifest(manifest);
        return -1;
    }

    const int record_result = write_package_record(manifest);
    if (record_result != 0)
    {
        free_manifest(manifest);
        return -1;
    }

    printf("Installed %s %s\n", manifest->name, manifest->version);

    free_manifest(manifest);
    return 0;
}

char* find_extracted_dir(const char* extract_dir)
{
    DIR* dir = opendir(extract_dir);
    if (dir == NULL)
    {
        perror("Failed to open extract directory");
        return NULL;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;
        if (entry->d_type == DT_DIR)
        {
            char* result = malloc(1024);
            if (result == NULL)
            {
                closedir(dir);
                return NULL;
            }
            snprintf(result, 1024, "%s/%s", extract_dir, entry->d_name);
            closedir(dir);
            return result;
        }
    }

    closedir(dir);
    return NULL;
}

int list(void)
{
    FILE* pF = fopen("xpkg.db", "r");
    if (pF == NULL)
    {
        fprintf(stderr, "No packages installed\n");
        return -1;
    }

    char line[2048];
    int count = 0;

    while (fgets(line, sizeof(line), pF) != NULL)
    {
        // format: name|version|files
        char* name = strtok(line, "|");
        char* version = strtok(NULL, "|");

        if (name != NULL && version != NULL)
        {
            printf("%s %s\n", name, version);
            count++;
        }
    }

    if (count == 0)
        printf("No packages installed\n");

    fclose(pF);
    return 0;
}

int remove_package(const char* package_name)
{
    FILE* pF = fopen("xpkg.db", "r");
    if (pF == NULL)
    {
        fprintf(stderr, "No packages installed\n");
        return -1;
    }

    char lines[256][2048];
    int line_count = 0;
    int found = 0;
    char files_str[2048] = "";

    while (fgets(lines[line_count], sizeof(lines[0]), pF) != NULL)
    {
        char temp[2048];
        strcpy(temp, lines[line_count]);

        char* name = strtok(temp, "|");
        if (name != NULL && strcmp(name, package_name) == 0)
        {
            found = 1;
            // extract files field
            strtok(NULL, "|"); // skip version
            const char* files = strtok(NULL, "|\n");
            if (files != NULL)
                strncpy(files_str, files, sizeof(files_str) - 1);
        }
        else
        {
            line_count++;
        }
    }
    fclose(pF);

    if (!found)
    {
        fprintf(stderr, "Package '%s' not found\n", package_name);
        return -1;
    }

    char* token = strtok(files_str, ",");
    while (token != NULL)
    {
        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "/tmp/xpkg_install/%s/%s", package_name, token);
        if (remove(file_path) != 0)
            fprintf(stderr, "Warning: could not delete %s\n", file_path);
        token = strtok(NULL, ",");
    }

    char dir_path[1024];
    snprintf(dir_path, sizeof(dir_path), "rm -rf /tmp/xpkg_install/%s", package_name);
    system(dir_path);

    FILE* pNew = fopen("xpkg.db", "w");
    if (pNew == NULL)
    {
        fprintf(stderr, "Failed to update database\n");
        return -1;
    }

    for (int i = 0; i < line_count; i++)
        fputs(lines[i], pNew);

    fclose(pNew);

    printf("Removed %s\n", package_name);
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

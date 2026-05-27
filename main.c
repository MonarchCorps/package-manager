#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
void free_manifest(Manifest* manifest);
void round_cleanup(
    Manifest* manifest,
    char* buffer,
    char* name,
    char* version,
    char** files
);

int main(void)
{
    Manifest* manifest = parse_manifest("manifest.json");
    if (manifest == NULL)
    {
        perror("error");
        return 1;
    }

    printf("%s\n", manifest->name);
    printf("%s\n", manifest->version);

    for (size_t i = 0; i < manifest->file_count; i++)
    {
        printf("%s\n", manifest->files[i]);
    }

    free_manifest(manifest);
    return 0;
}

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

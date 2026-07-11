#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>

#define DEFAULT_CHUNK_SIZE 1024
#define MAX_CHUNK_SIZE 8192

char *buf;
char *pattern;

void read_mode(size_t chunkSize, int infile);
void mmap_mode(int infile);

int main(int argc, char *argv[]) {
    size_t chunkSize;
    int infile;

    if (argc < 3) {
        printf("Usage:\n\t");
        printf("./proj2 srcfile searchstring [size|mmap]\n");
        exit(1);
    }

    if (argc > 3) {
        if (strcmp(argv[3], "mmap") == 0) {
            chunkSize = 0;
        } else {
            chunkSize = atoi(argv[3]);
        }
        if (chunkSize > MAX_CHUNK_SIZE) {
            printf("Chunk size exceeded limit (%d bytes). ", MAX_CHUNK_SIZE);
            printf("Defaulting to %d bytes\n", DEFAULT_CHUNK_SIZE);
            chunkSize = DEFAULT_CHUNK_SIZE;
        }
    } else {
        chunkSize = DEFAULT_CHUNK_SIZE;
    }

    if ((infile = open(argv[1], O_RDONLY)) == -1) {
        printf("Failed to open file: %s\n", argv[1]);
        exit(1);
    }

    pattern = argv[2];

    if (chunkSize == 0) {
        mmap_mode(infile);
    } else {
        read_mode(chunkSize, infile);
    }

    return 0;
}

void read_mode(size_t chunkSize, int infile) {
    int matches, searchIndex, fileSize, charsRead;
    ssize_t bufIndex;

    printf("Entering read mode with chunkSize = %zu bytes\n", chunkSize);

    buf = (char *)malloc(chunkSize * sizeof(char));

    matches = 0;
    searchIndex = 0;
    fileSize = 0;
    bufIndex = 0;
    char *readBuffer = (char *)malloc(chunkSize * sizeof(char));
    while ((charsRead = read(infile, readBuffer, chunkSize))) {
        fileSize += charsRead;
        for (int i = 0; i < charsRead; i++) {
            buf[bufIndex++] = readBuffer[i];
            if (pattern[searchIndex] == '\0') {
                matches++;
                searchIndex = 0;
            } else if (buf[bufIndex] == pattern[searchIndex]) {
                searchIndex++;
            } else {
                searchIndex = 0;
            }
        }
    }
    close(infile);
    free(readBuffer);

    printf("File size: %d bytes.\n", fileSize);
    printf("Occurrences of the string \"%s\": %d\n", pattern, matches);
}

void mmap_mode(int infile) {
    struct stat sb;

    printf("Entering mmap mode\n");

    if (fstat(infile, &sb) < 0) {
        printf("Failed to stat file\n");
        exit(1);
    }

    if ((buf = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, infile, 0)) == (char *)-1) {
        printf("Failed to mmap file\n");
        exit(1);
    }

    int matches = 0;
    int searchIndex = 0;

    for (int i = 0; i < sb.st_size; i++) {
        if (pattern[searchIndex] == '\0') {
            matches++;
            searchIndex = 0;
        } else if (buf[i] == pattern[searchIndex]) {
            searchIndex++;
        } else {
            searchIndex = 0;
        }
    }

    if (munmap(buf, sb.st_size) < 0) {
        printf("Failed to unmap memory\n");
        exit(1);
    }

    close(infile);

    printf("Occurrences of the string \"%s\": %d\n", pattern, matches);
}
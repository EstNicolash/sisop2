#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#define MAX_FILENAME_SIZE 1024
#include <stdint.h>
#include <time.h>

typedef struct FileInfo {
  char filename[MAX_FILENAME_SIZE];
  uint64_t file_size;
  time_t last_modified;
  time_t last_accessed;
  time_t creation_time;
} FileInfo;

bool create_directory(const char dir_name[MAX_FILENAME_SIZE]);
bool save_file(char *file_buffer);
char *load_file(const char file_name[MAX_FILENAME_SIZE]);
FileInfo *list_files(const char dir_name[MAX_FILENAME_SIZE], int *file_count);
bool delete_file(const char file_name[MAX_FILENAME_SIZE]);

#endif

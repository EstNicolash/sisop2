#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#define MAX_FILENAME_SIZE 1024
#include <stdint.h>
#include <time.h>

// File metadata
typedef struct FileInfo {
  char filename[MAX_FILENAME_SIZE];
  uint64_t file_size;
  time_t last_modified;
  time_t last_accessed;
  time_t creation_time;
} FileInfo;

// Create a directory by it name
int create_directory(const char dir_name[MAX_FILENAME_SIZE]);

// Save a char stream as a file in the specified directory
int save_file(const char file_name[MAX_FILENAME_SIZE],
              const char dir_name[MAX_FILENAME_SIZE], char *file_buffer,
              size_t buffer_size);

// Load a file as a char stream and return the stream
char *load_file(const char file_name[MAX_FILENAME_SIZE], size_t *file_size);

// Delete a file
int delete_file(const char file_name[MAX_FILENAME_SIZE]);

// Get filemetada
FileInfo get_file_info(const char file_name[MAX_FILENAME_SIZE]);

// Lists all files' metadata of specified direcotry
FileInfo *list_files(const char dir_name[MAX_FILENAME_SIZE], int *file_count);

#endif

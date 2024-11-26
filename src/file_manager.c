#include "../headers/file_manager.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int create_directory(const char dir_name[MAX_FILENAME_SIZE]) {
  return mkdir(dir_name, 0755);
}

int file_exists(const char filename[MAX_FILENAME_SIZE]) {
  FILE *file = fopen(filename, "r");
  if (file) {
    fclose(file);
    return 0;
  }
  return -1;
}
int save_file(const char file_name[MAX_FILENAME_SIZE],
              const char dir_name[MAX_FILENAME_SIZE], char *file_buffer,
              size_t buffer_size) {
  char file_path[MAX_FILENAME_SIZE * 2];
  snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, file_name);

  // Ensure directory exists
  struct stat st;
  if (stat(dir_name, &st) == -1) {
    if (mkdir(dir_name, 0755) == -1) {
      perror("Failed to create directory");
      return -1;
    }
  }

  int fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("Failed to open file for writing");
    return -1;
  }

  // Write file in a loop to ensure all data is written
  size_t total_written = 0;
  while (total_written < buffer_size) {
    ssize_t bytes_written =
        write(fd, file_buffer + total_written, buffer_size - total_written);
    if (bytes_written == -1) {
      perror("Failed to write to file");
      close(fd);
      return -1;
    }
    total_written += bytes_written;
  }

  close(fd);

  //    printf("File saved successfully: %s (size: %zu bytes)\n", file_path,
  //    buffer_size);
  return 0;
}

int delete_file(const char file_name[MAX_FILENAME_SIZE]) {
  return unlink(file_name);
}

FileInfo get_file_info(const char file_name[MAX_FILENAME_SIZE]) {
  FileInfo info = {0};
  struct stat st;

  if (stat(file_name, &st) == -1) {
    perror("Failed to get file metadata");
    return info;
  }

  strncpy(info.filename, file_name, MAX_FILENAME_SIZE);
  info.file_size = st.st_size;
  info.last_modified = st.st_mtime;
  info.last_accessed = st.st_atime;
  info.creation_time = st.st_ctime;

  return info;
}

FileInfo *list_files(const char dir_name[MAX_FILENAME_SIZE], int *file_count) {
  DIR *dir = opendir(dir_name);
  if (!dir) {
    perror("Failed to open directory");
    return NULL;
  }

  struct dirent *entry;
  struct stat st;
  FileInfo *files = NULL;
  *file_count = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type != DT_REG)
      continue;

    char file_path[MAX_FILENAME_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name);

    if (stat(file_path, &st) == -1) {
      perror("Failed to get file metadata");
      continue;
    }

    files = realloc(files, sizeof(FileInfo) * (*file_count + 1));
    if (!files) {
      perror("Failed to allocate memory for file list");
      closedir(dir);
      return NULL;
    }

    strncpy(files[*file_count].filename, entry->d_name, MAX_FILENAME_SIZE);
    files[*file_count].file_size = st.st_size;
    files[*file_count].last_modified = st.st_mtime;
    files[*file_count].last_accessed = st.st_atime;
    files[*file_count].creation_time = st.st_ctime;
    (*file_count)++;
  }

  closedir(dir);
  return files;
}

void print_file_list(FileInfo *files, int num_files) {
  if (!files || num_files <= 0) {
    printf("No files to display.\n");
    return;
  }

  printf("File List:\n");
  printf("---------------------------------------------------------------------"
         "-----\n");
  printf("| %-30s | %-10s | %-20s |\n", "Filename", "Size (bytes)",
         "Last Modified");
  printf("---------------------------------------------------------------------"
         "-----\n");

  for (int i = 0; i < num_files; i++) {
    char last_modified_str[20];
    struct tm *tm_info = localtime(&files[i].last_modified);
    strftime(last_modified_str, sizeof(last_modified_str), "%Y-%m-%d %H:%M:%S",
             tm_info);

    printf("| %-30s | %-10lu | %-20s |\n", files[i].filename,
           files[i].file_size, last_modified_str);
  }

  printf("---------------------------------------------------------------------"
         "-----\n");
}

#include "../headers/file_manager.h"
#include <stdio.h>
int main() {
  const char dir_name[MAX_FILENAME_SIZE] = "../obj";
  const char file_name[MAX_FILENAME_SIZE] = "articulo.pdf";

  size_t len = 0;
  char *file_buffer = load_file(file_name, &len);

  if (save_file(file_name, dir_name, file_buffer, len) == 0) {
    printf("File saved successfully.\n");
  } else {
    printf("Failed to save file.\n");
  }

  const char deleten[MAX_FILENAME_SIZE] = "../obj/articulo.pdf";
  if (delete_file(deleten) == 0) {

    printf("File deleted successfully.\n");
  }
}

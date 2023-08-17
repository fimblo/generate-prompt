#include <stdio.h>
#include <string.h>
#include <git2.h>

int findGitRepository(const char *path) {
  git_buf repo_path = { 0 };
  int error = git_repository_discover(&repo_path, path, 0, NULL);

  if (error == 0) {
    char *last_slash = strstr(repo_path.ptr, "/.git/");
    if (last_slash) {
      *last_slash = '\0';  // Null-terminate the string at the last slash
    }
    printf("%s\n", repo_path.ptr);
    git_buf_free(&repo_path);
    return 1;
  }

  // Check if we've reached the file system root
  if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0) {
    return 0;
  }

  // Move to the parent directory
  char *last_slash = strrchr(path, '/');
  if (last_slash) {
    char parent_path[last_slash - path + 1];
    strncpy(parent_path, path, last_slash - path);
    parent_path[last_slash - path] = '\0';

    return findGitRepository(parent_path);
  }
  else {
    return 0;
  }
}

int main() {
  git_libgit2_init();

  int result = findGitRepository(".");

  git_libgit2_shutdown();

  return result;
}

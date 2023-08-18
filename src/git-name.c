#include <stdio.h>
#include <string.h>
#include <git2.h>

const char* findGitRepoName(const char* path) {
  git_buf repo_path = { 0 };
  int error = git_repository_discover(&repo_path, path, 0, NULL);

  if (error == 0) {
    git_repository *repo = NULL;
    if (git_repository_open(&repo, repo_path.ptr) == 0) {


      const char *full_repo_dir = git_repository_commondir(repo);
      size_t full_repo_dir_len = strlen(full_repo_dir);
      char *repo_dir = "";

      if (full_repo_dir_len >= 6 && strcmp(full_repo_dir + full_repo_dir_len - 6, "/.git/") == 0) {
        repo_dir = strndup(full_repo_dir, full_repo_dir_len - 6); // Remove the trailing "/.git/"
      }

      const char *last_slash = strrchr(repo_dir, '/');
      
      if (last_slash) {
        const char *repo_name = last_slash + 1;
        git_repository_free(repo);
        git_buf_dispose(&repo_path);

        char * retval = strdup(repo_name);
        free(repo_dir);
        return retval;
      }
      free(repo_dir);
      git_repository_free(repo);
    } else {
      printf("Error opening repository: %s\n", git_error_last()->message);
    }
  }

  git_buf_dispose(&repo_path);
  return strdup("");
}

int main() {
  git_libgit2_init();

  const char* repo_name = findGitRepoName(".");

  if (repo_name && strlen(repo_name) > 0) {
    printf("%s\n", repo_name);
  }

  free(repo_name);
  git_libgit2_shutdown();

  return 0;
}

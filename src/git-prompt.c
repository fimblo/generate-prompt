// includes
#include <stdio.h>
#include <string.h>
#include <git2.h>

// defines
#define COLOR_UP_TO_DATE "\033[0;32m"  // cyan
#define COLOR_MODIFIED   "\033[01;33m" // bold yellow
#define COLOR_STAGED     "\033[01;31m" // bold red
#define COLOR_CWD        "\033[1;34m"  // blue
#define COLOR_RESET      "\033[0m"

// declarations
const char* findGitRepoName(const char* path);
const char* findGitRepository(const char *path);
void printDefaultPrompt();



int main() {
  git_libgit2_init();

  // get path to git repo at '.' else return
  const char *git_repository_path = findGitRepository(".");
  if (strlen(git_repository_path) == 0) {
    printDefaultPrompt();
    return 0;
  }

  // get repo else return
  git_repository *repo = NULL;
  if (git_repository_open(&repo, git_repository_path) != 0) {
    printDefaultPrompt();
    return 0;
  }

  // ref to current HEAD of repo else return
  git_reference *head_ref = NULL;
  if (git_repository_head(&head_ref, repo) != 0) {
    printDefaultPrompt();
    git_repository_free(repo);
    return 1;
  }

  // get repo and branch name
  const char *repo_name = findGitRepoName(".");
  const char *branch_name = git_reference_shorthand(head_ref);

  const char *prompt_format = "\[%s\]\[%s%s%s\] %s $ ";

  char prompt_cwd[256];
  snprintf(prompt_cwd, sizeof(prompt_cwd), "%s\\W%s", COLOR_CWD, COLOR_RESET);

  char prompt_up_to_date[512];
  snprintf(prompt_up_to_date, sizeof(prompt_up_to_date),
           prompt_format,
           repo_name,
           COLOR_UP_TO_DATE,
           branch_name,
           COLOR_RESET,
           prompt_cwd);
  char prompt_modified[512];
  snprintf(prompt_modified, sizeof(prompt_modified),
           prompt_format,
           repo_name,
           COLOR_MODIFIED,
           branch_name,
           COLOR_RESET,
           prompt_cwd);
  char prompt_staged[512];
  snprintf(prompt_staged, sizeof(prompt_staged),
           prompt_format,
           repo_name,
           COLOR_STAGED,
           branch_name,
           COLOR_RESET,
           prompt_cwd);

  // set up git status 
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo, &opts) != 0) {
    git_reference_free(head_ref);
    git_repository_free(repo);
    return 1;
  }

  int status_count = git_status_list_entrycount(status_list);
  if (status_count == 0) {
    printf("%s",prompt_up_to_date);
  } else {
    int i;
    int staged_changes = 0;
    int unstaged_changes = 0;
    for (i = 0; i < status_count; i++) {
      const git_status_entry *entry = git_status_byindex(status_list, i);
      if (entry->status == GIT_STATUS_INDEX_NEW || entry->status == GIT_STATUS_INDEX_MODIFIED) {
        staged_changes = 1;
      }
      if (entry->status == GIT_STATUS_WT_MODIFIED) {
        unstaged_changes = 1;
      }
    }
    if (staged_changes) {
      printf("%s",prompt_staged);
    } else if (unstaged_changes) {
      printf("%s",prompt_modified);
    } else {
      // special case: we have staged AND modified.
      // in this case, colour as if staged
      printf("%s",prompt_staged);
    }
  }

  git_status_list_free(status_list);
  git_reference_free(head_ref);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
}


// return repo name else null
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
    }
  }

  git_buf_dispose(&repo_path);
  return NULL;
}




// Return path to repo else empty string
const char* findGitRepository(const char *path) {
  git_buf repo_path = { 0 };
  int error = git_repository_discover(&repo_path, path, 0, NULL);

  if (error == 0) {
    char *last_slash = strstr(repo_path.ptr, "/.git/");
    if (last_slash) {
      *last_slash = '\0';  // Null-terminate the string at the last slash
    }
    char *result = strdup(repo_path.ptr);  // Duplicate the path before freeing the buffer
    git_buf_free(&repo_path);
    return result;
  }

  // Check if we've reached the file system root
  if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0) {
    return strdup("");
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
    return strdup("");
  }
}


void printDefaultPrompt() {
  const char *defaultPrompt = getenv("DEFAULT_PROMPT");
  if (defaultPrompt) {
    printf("%s", defaultPrompt);
    return;
  } 

  char prompt[256];
  snprintf(prompt, sizeof(prompt), "%s\\W%s $ ", COLOR_CWD, COLOR_RESET);
  printf("%s", prompt);
}



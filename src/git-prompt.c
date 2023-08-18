// includes
#include <stdio.h>
#include <string.h>
#include <git2.h>

// defines

#define up_to_date 0
#define modified   1
#define staged     2
#define cwd        3
#define reset      4

const char *color[5] = {
  "\033[0;32m",  // up_to_date - cyan
  "\033[01;33m", // modified   - bold yellow
  "\033[01;31m", // staged     - bold red
  "\033[1;34m",  // cwd        - blue
  "\033[0m"      // reset      - reset to default
};

// declarations
const char* findGitRepository(const char *path);
void printNonGitPrompt();
void printPrompt(const char *repo_name, const char *branch_name, const int status);







int main() {
  git_libgit2_init();

  // get path to git repo at '.' else print default prompt
  const char *git_repository_path = findGitRepository(".");      // "/path/to/projectName"
  if (strlen(git_repository_path) == 0) {
    printNonGitPrompt();
    return 0;
  }


  // if we can't create repo object, print default prompt
  git_repository *repo = NULL;
  if (git_repository_open(&repo, git_repository_path) != 0) {
    printNonGitPrompt();
    return 0;
  }

  // if we can't get ref to repo, print defualt prompt
  git_reference *head_ref = NULL;
  if (git_repository_head(&head_ref, repo) != 0) {
    printNonGitPrompt();
    git_repository_free(repo);
    return 1;
  }

  // get repo name and branch name
  const char *repo_name = strrchr(git_repository_path, '/') + 1; // "projectName"
  const char *branch_name = git_reference_shorthand(head_ref);



  // set up git status
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo, &opts) != 0) {
    git_reference_free(head_ref);
    git_repository_free(repo);
    printNonGitPrompt();
    return 1;
  }

  int status_count = git_status_list_entrycount(status_list);
  if (status_count == 0) {
    printPrompt(repo_name, branch_name, up_to_date);
  } else {
    int i;
    int staged_changes = 0;
    int modified_changes = 0;
    for (i = 0; i < status_count; i++) {
      const git_status_entry *entry = git_status_byindex(status_list, i);
      if (entry->status == GIT_STATUS_INDEX_NEW || entry->status == GIT_STATUS_INDEX_MODIFIED) {
        staged_changes = 1;
      }
      if (entry->status == GIT_STATUS_WT_MODIFIED) {
        modified_changes = 1;
      }
    }
    if (staged_changes) {
      printPrompt(repo_name, branch_name, staged);
    } else if (modified_changes) {
      printPrompt(repo_name, branch_name, modified);
    } else {
      // special case: we have staged AND modified.
      // in this case, colour as if staged
      printPrompt(repo_name, branch_name, staged);
    }
  }

  git_status_list_free(status_list);
  git_reference_free(head_ref);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
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

void printNonGitPrompt() {
  const char *defaultPrompt = getenv("DEFAULT_PROMPT");
  if (defaultPrompt) {
    printf("%s", defaultPrompt);
    return;
  }

  char prompt[256];
  snprintf(prompt, sizeof(prompt), "%s\\W%s $ ", color[cwd], color[reset]);
  printf("%s", prompt);
}


void printPrompt(const char *repo_name, const char *branch_name, const int status) {
  const char *format = "\[%s\]\[%s%s%s\] %s\\W%s $ ";

  char prompt_status[512];
  snprintf(prompt_status, sizeof(prompt_status),
           format,
           repo_name,
           color[status],
           branch_name,
           color[reset],
           color[cwd],
           color[reset]);

  printf("%s", prompt_status);
}


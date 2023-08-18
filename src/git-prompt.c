/* -------------------------------------------------- */
// includes
#include <stdio.h>
#include <string.h>
#include <git2.h>


/* -------------------------------------------------- */
// enums and colors
enum states {
  UP_TO_DATE = 1<<0,
  MODIFIED   = 1<<1,
  STAGED     = 1<<2,
  CWD        = 1<<3,
  RESET      = 1<<4,
};

const char *color[1<<5] = {
  [ UP_TO_DATE ] =  "\033[0;32m",  // UP_TO_DATE - cyan
  [ MODIFIED   ] =  "\033[01;33m", // MODIFIED   - bold yellow
  [ STAGED     ] =  "\033[01;31m", // STAGED     - bold red
  [ CWD        ] =  "\033[1;34m",  // CWD        - blue
  [ RESET      ] =  "\033[0m"      // RESET      - RESET to default
};


/* -------------------------------------------------- */
// declarations
const char* findGitRepository(const char *path);
void printNonGitPrompt();
void printPrompt(const char *repo_name, const char *branch_name, const int status);
size_t stripped_strlen(const char *str);





/* -------------------------------------------------- */
// functions
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
  int status = 0;
  if (status_count == 0) {
    status |= UP_TO_DATE;
  }
  else {

    for (int i = 0; i < status_count; i++) {
      const git_status_entry *entry = git_status_byindex(status_list, i);
      if (entry->status == GIT_STATUS_INDEX_NEW || entry->status == GIT_STATUS_INDEX_MODIFIED) {
        status |= STAGED;
      }
      if (entry->status == GIT_STATUS_WT_MODIFIED) {
        status |= MODIFIED;
      }
    }
  }
  printPrompt(repo_name, branch_name, status);

  git_status_list_free(status_list);
  git_reference_free(head_ref);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
}


/* -------------------------------------------------- */
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

/* -------------------------------------------------- */
// When standing in a non-git repo, or if the git prompt doesn't work,
// use this prompt.
void printNonGitPrompt() {
  const char *defaultPrompt = getenv("DEFAULT_PROMPT");
  if (defaultPrompt) {
    printf("%s", defaultPrompt);
    return;
  }

  char prompt[256];
  snprintf(prompt, sizeof(prompt), "%s\\W%s $ ", color[CWD], color[RESET]);
  printf("%s", prompt);
}


/* -------------------------------------------------- */
// When standing in a git-repo, use this prompt
void printPrompt(const char *repo_name, const char *branch_name, const int status) {
  const char *format_top    = "╭── \[%s\]\[%s%s%s\] %s\\W%s ";
  const char *format_bottom = "╰➧$ ";

  // figure out what status to use
  int opt = 0;
  if ((status & MODIFIED) && (status & STAGED)) {
    opt |= STAGED;
  }
  else {
    opt = status;
  }

  char top_prompt[512];
  snprintf(top_prompt, sizeof(top_prompt),
           format_top,
           repo_name,
           color[opt],
           branch_name,
           color[RESET],
           color[CWD],
           color[RESET]);


  printf("%s\n", top_prompt);
  printf("%s",format_bottom);

}

size_t stripped_strlen(const char *str) {
  size_t len = 0;
  int in_escape = 0;

  for (const char *p = str; *p != '\0'; ++p) {
    if (*p == '\033') { // Check for escape character
      in_escape = 1;
    } else if (in_escape && *p == 'm') { // Check for end of escape sequence
      in_escape = 0;
    } else if (!in_escape) { // If not in escape sequence, count as character
      len++;
    }
  }

  return len;
}

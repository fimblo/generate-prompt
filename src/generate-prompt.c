/* --------------------------------------------------
 * Includes
 */
#include <stdio.h>
#include <string.h>
#include <git2.h>


/* --------------------------------------------------
 * Enums and colors
 */
enum states {
  UP_TO_DATE = 1<<0,
  MODIFIED   = 1<<1,
  STAGED     = 1<<2,
  CWD        = 1<<3,
  RESET      = 1<<4,
};

const char *color[1<<5];


/* --------------------------------------------------
 * Declarations
 */
const char* findGitRepositoryPath(const char *path);
char* substitute (const char * text, const char * search, const char * replacement);
void printNonGitPrompt();
void printGitPrompt(const char *repo_name, const char *branch_name, const int rstatus, const int istatus, const int wstatus);
void setup_colors();
char* replace(const char* input, const char* repo_name, const char* branch_name, const int rstatus, const int istatus, const int wstatus);

/* --------------------------------------------------
 * Functions
 */
int main() {
  git_libgit2_init();

  // get path to git repo at '.' else print default prompt
  const char *git_repository_path = findGitRepositoryPath(".");      // "/path/to/projectName"
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
    git_libgit2_shutdown();
    printNonGitPrompt();
    return 1;
  }

  // get repo name and branch names
  const char *repo_name = strrchr(git_repository_path, '/') + 1; // "projectName"
  const char *branch_name = git_reference_shorthand(head_ref);
  char full_local_branch_name[128];
  sprintf(full_local_branch_name, "refs/heads/%s",  branch_name);


  // check if local and remote are the same
  const git_oid *local_branch_commit_id = git_reference_target(head_ref);

  char full_remote_branch_name[128];
  sprintf(full_remote_branch_name, "refs/remotes/origin/%s", git_reference_shorthand(head_ref));

  git_reference *upstream_ref = NULL;
  if (git_reference_lookup(&upstream_ref, repo, full_remote_branch_name)) {
    printNonGitPrompt();
    git_reference_free(head_ref);
    git_repository_free(repo);
    git_libgit2_shutdown();
    return 1;
  }
  const git_oid *remote_commit_id = git_reference_target(upstream_ref);

  int rstatus = UP_TO_DATE;
  if (git_oid_cmp(local_branch_commit_id, remote_commit_id) != 0)
    rstatus = MODIFIED; 


  // set up git status
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo, &opts) != 0) {
    git_reference_free(upstream_ref);
    git_reference_free(head_ref);
    git_repository_free(repo);
    printNonGitPrompt();
    return 1;
  }

  int status_count = git_status_list_entrycount(status_list);
  int istatus = UP_TO_DATE;
  int wstatus = UP_TO_DATE;
  if (status_count != 0) {
    for (int i = 0; i < status_count; i++) {
      const git_status_entry *entry = git_status_byindex(status_list, i);
      if (entry->status == GIT_STATUS_CURRENT)         continue;

      if (entry->status & GIT_STATUS_INDEX_NEW)        istatus = STAGED;
      if (entry->status & GIT_STATUS_INDEX_MODIFIED)   istatus = STAGED;
      if (entry->status & GIT_STATUS_INDEX_RENAMED)    istatus = STAGED;
      if (entry->status & GIT_STATUS_INDEX_DELETED)    istatus = STAGED;
      if (entry->status & GIT_STATUS_INDEX_TYPECHANGE) istatus = STAGED;

      if (entry->status & GIT_STATUS_WT_RENAMED)       wstatus = MODIFIED;
      if (entry->status & GIT_STATUS_WT_DELETED)       wstatus = MODIFIED;
      if (entry->status & GIT_STATUS_WT_MODIFIED)      wstatus = MODIFIED;
      if (entry->status & GIT_STATUS_WT_TYPECHANGE)    wstatus = MODIFIED;
    }
  }
  printGitPrompt(repo_name, branch_name, rstatus, istatus, wstatus);

  git_status_list_free(status_list);
  git_reference_free(upstream_ref);
  git_reference_free(head_ref);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
}


/* --------------------------------------------------
 * Return path to repo else empty string
 */
const char* findGitRepositoryPath(const char *path) {
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

    return findGitRepositoryPath(parent_path);
  }
  else {
    return strdup("");
  }
}

/* --------------------------------------------------
 * When standing in a non-git repo, or if the git prompt doesn't work,
 * use this prompt.
 */
void printNonGitPrompt() {
  setup_colors();

  const char *defaultPrompt = getenv("GP_DEFAULT_PROMPT");
  if (defaultPrompt) {
    printf("%s", defaultPrompt);
    return;
  }

  printf( "%s\\W%s $ ", color[CWD], color[RESET]);
}


void printGitPrompt(const char *repo_name, const char *branch_name, const int rstatus, const int istatus, const int wstatus) {
  setup_colors();

  const char* input = getenv("GP_GIT_PROMPT");
  char* output = replace(input, repo_name, branch_name, rstatus, istatus, wstatus);

  printf("%s", output);
  free(output);
}


/*
 * Helper function to set all colors
 */
void setup_colors() {
  color[ UP_TO_DATE ] = getenv("GP_UP_TO_DATE") ?: "\033[0;32m";  // UP_TO_DATE - default cyan
  color[ MODIFIED   ] = getenv("GP_MODIFIED")   ?: "\033[01;33m"; // MODIFIED   - default bold yellow
  color[ STAGED     ] = getenv("GP_STAGED")     ?: "\033[01;31m"; // STAGED     - default bold red
  color[ CWD        ] = getenv("GP_CWD")        ?: "\033[1;34m";  // CWD        - default blue
  color[ RESET      ] = "\033[0m"; // RESET      - RESET to default
}

/*
  Function will look for the strings \pR, \pB, and \pC
  Each, if found in `input`, will be replaced like so:

  \pR: Replaced with repo_name
  \pB: Replaced with "%s%s%s", color[istatus], branch_name, color[reset]
  \pC: Replaced with "%s \w %s", color[istatus], color[reset]

  Expect input like this:  "[\pR] [\pB] [\pC]"
*/
char* replace(const char* input, const char* repo_name, const char* branch_name, const int rstatus, const int istatus, const int wstatus) {
  char repo_temp[256];
  char branch_temp[256];
  char cwd_temp[256];
  sprintf(repo_temp, "%s%s%s", color[rstatus], repo_name, color[RESET]);
  sprintf(branch_temp, "%s%s%s", color[istatus], branch_name, color[RESET]);
  sprintf(cwd_temp, "%s\\W%s", color[wstatus], color[RESET]);

  const char* searchStrings[] = { "\\pR", "\\pB", "\\pC" };
  const char* replaceStrings[] = { repo_temp, branch_temp, cwd_temp };

  char* output = strdup(input);

  for (int i = 0; i < sizeof(searchStrings) / sizeof(searchStrings[0]); i++) {
    const char* searchString = searchStrings[i];
    const char* replaceString = replaceStrings[i];
    output = substitute(output, searchString, replaceString);
  }

  return output;
}




/* --------------------------------------------------
 * Helper: substitute
 */
char* substitute(const char* text, const char* search, const char* replacement) {
  char* message = strdup(text);
  char* found = strstr(message, search);

  while (found) {
    size_t prefix_length = found - message;
    size_t suffix_length = strlen(found + strlen(search));

    size_t new_length = prefix_length + strlen(replacement) + suffix_length + 1;
    char* temp = malloc(new_length);  // Allocate temporary buffer for the new string

    strncpy(temp, message, prefix_length);
    temp[prefix_length] = '\0';

    strcat(temp, replacement);

    // If there's still a suffix, copy it to the new string
    if (suffix_length > 0) {
      strcat(temp, found + strlen(search));
    }

    free(message);  // Free the old 'message'
    message = temp;  // Set 'message' to the new string

    found = strstr(message, search);  // Search for the next occurrence
  }

  return message;
}

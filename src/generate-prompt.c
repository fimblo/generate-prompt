/* --------------------------------------------------
 * Includes
 */
#include <stdio.h>
#include <string.h>
#include <git2.h>
#include <unistd.h>
#include <libgen.h>

/* --------------------------------------------------
 * Common global stuff
 */
enum states {
  UP_TO_DATE = 0,
  MODIFIED   = 1,
  RESET      = 2,
};

struct GitStatus {
  const char *repo_name;
  const char *branch_name;
  const char *repo_path;
  int repo;
  int index;
  int wdir;
};


/* --------------------------------------------------
 * Declarations
 */
void printNonGitPrompt();
void printGitPrompt(const struct GitStatus *status);

// helpers
const char* findGitRepositoryPath(const char *path);
char* replace(const char* input, const struct GitStatus *status);
char* substitute (const char * text, const char * search, const char * replacement);

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
  const git_oid *local_commit_id = git_reference_target(head_ref);

  char full_remote_branch_name[128];
  sprintf(full_remote_branch_name, "refs/remotes/origin/%s", git_reference_shorthand(head_ref));

  git_reference *upstream_ref = NULL;
  if (git_reference_lookup(&upstream_ref, repo, full_remote_branch_name)) {
    git_reference_free(head_ref);
    git_repository_free(repo);
    git_libgit2_shutdown();
    printNonGitPrompt();
    return 1;
  }
  const git_oid *remote_commit_id = git_reference_target(upstream_ref);


  struct GitStatus status;
  status.repo_name   = repo_name;
  status.branch_name = branch_name;
  status.repo_path   = git_repository_path;
  status.repo        = UP_TO_DATE;
  status.index       = UP_TO_DATE;
  status.wdir        = UP_TO_DATE;

  if (git_oid_cmp(local_commit_id, remote_commit_id) != 0)
    status.repo = MODIFIED;

  // set up git status
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo, &opts) != 0) {
    git_reference_free(upstream_ref);
    git_reference_free(head_ref);
    git_repository_free(repo);
    git_libgit2_shutdown();
    printNonGitPrompt();
    return 1;
  }

  // check index and wt for diffs
  int status_count = git_status_list_entrycount(status_list);
  if (status_count != 0) {
    for (int i = 0; i < status_count; i++) {
      const git_status_entry *entry = git_status_byindex(status_list, i);
      if (entry->status == GIT_STATUS_CURRENT)         continue;

      if (entry->status & GIT_STATUS_INDEX_NEW)        status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_MODIFIED)   status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_RENAMED)    status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_DELETED)    status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_TYPECHANGE) status.index = MODIFIED;

      if (entry->status & GIT_STATUS_WT_RENAMED)       status.wdir = MODIFIED;
      if (entry->status & GIT_STATUS_WT_DELETED)       status.wdir = MODIFIED;
      if (entry->status & GIT_STATUS_WT_MODIFIED)      status.wdir = MODIFIED;
      if (entry->status & GIT_STATUS_WT_TYPECHANGE)    status.wdir = MODIFIED;
    }
  }

  // print the git prompt now that we have the info
  printGitPrompt(&status);

  // clean up before we end
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
  const char *defaultPrompt = getenv("GP_DEFAULT_PROMPT");
  if (defaultPrompt) {
    printf("%s", defaultPrompt);
    return;
  }

  printf( "\\W $ ");
}

/* --------------------------------------------------
 *  When standing in a git repo, use this prompt.
 */
void printGitPrompt(const struct GitStatus *status) {
  const char* input = getenv("GP_GIT_PROMPT");
  char* output = replace(input, status);

  printf("%s", output);
  free(output);
}


/* --------------------------------------------------
 * printGitPrompt helper
 */
char* replace(const char* input, const struct GitStatus *status) {
  const char *colour[3];
  colour[ UP_TO_DATE ] = getenv("GP_UP_TO_DATE") ?: "\033[0;32m";  // UP_TO_DATE - default green
  colour[ MODIFIED   ] = getenv("GP_MODIFIED")   ?: "\033[0;33m";  // MODIFIED   - default yellow
  colour[ RESET      ] = "\033[0m"; // RESET      - RESET to default


  char wd[2048];
  char cwd[2048];
  getcwd(cwd, sizeof(cwd));

  const char *wd_style = getenv("GP_GIT_WD_STYLE") ?: "basename";

  if (strcmp(wd_style, "cwd") == 0) {
    // show the entire path, from $HOME
    const char * home = getenv("HOME") ?: "";
    size_t common_length = strspn(cwd, home);
    sprintf(wd, "~/%s", cwd + common_length);
  }
  else if (strcmp(wd_style, "relpath") == 0) { //todo when with_root is added, rename this to _no_root
    // show the entire path, from git-root (exclusive)
    size_t common_length = strspn(status->repo_path, cwd);
    sprintf(wd, "%s", cwd + common_length);
    if (strlen(wd) == 0) {
      sprintf(wd, "//");
    }
  }
  /* else if (strcmp(wd_style, "relpath_with_root") == 0) { */
  /*   // show the entire path, from git-root (inclusive) */
  /*   size_t common_length = strspn(status->repo_path, cwd); */
  /*   sprintf(wd, "%s", cwd + common_length); */
  /* } */
  else {
    // basename - show current directory only
    sprintf(wd, "%s", basename(cwd));
  }



  char repo_temp[256];
  char branch_temp[256];
  char cwd_temp[2048];
  sprintf(repo_temp, "%s%s%s", colour[status->repo], status->repo_name, colour[RESET]);
  sprintf(branch_temp, "%s%s%s", colour[status->index], status->branch_name, colour[RESET]);
  sprintf(cwd_temp, "%s%s%s", colour[status->wdir], wd, colour[RESET]);

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

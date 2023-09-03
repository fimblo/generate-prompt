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
  UP_TO_DATE  = 0,
  MODIFIED    = 1,
  NO_DATA     = 2,
  RESET       = 3,
};

// used to pass repo info around between functions
struct RepoStatus {
  const char *repo_name;
  const char *branch_name;
  const char *repo_path;
  int repo;
  int index;
  int wdir;
  int ahead;
  int behind;
};


/* --------------------------------------------------
 * Declarations
 */


/*
  Prints a prompt which is useful outside of git repositories.
  Respects the environment variable GP_DEFAULT_PROMPT to override the
  built-in version of the prompt.
*/
void printNonGitPrompt();


/*
  Prints a prompt which is useful inside of git repositories.
  Respects the environment variable GP_GIT_PROMPT to override the
  built-in version of the prompt.
*/
void printGitPrompt(const struct RepoStatus *repo_status);


/*
  Given a path, looks recursively down toward the root of the
  filesystem for a git project folder.

  Returns absolute path to the git repository, sans the '.git/'
  directory.
*/
const char* findGitRepositoryPath(const char *path);


/*
  Returns how far ahead/behind local repo is when compared to upstream
 */
int calculateAheadBehind(git_repository *repo,
                         const git_oid *local_oid,
                         const git_oid *upstream_oid,
                         int *ahead,
                         int *behind);


/*
  Helper for doing the actual substitution.
*/
char* substitute (const char * text, const char * search, const char * replacement);



/* --------------------------------------------------
 * Functions
 */
int main() {
  git_libgit2_init();

  struct RepoStatus repo_status;
  repo_status.repo        = UP_TO_DATE;
  repo_status.index       = UP_TO_DATE;
  repo_status.wdir        = UP_TO_DATE;
  repo_status.ahead       = 0;
  repo_status.behind      = 0;

  // get path to git repo at '.' else print default prompt
  const char *git_repository_path = findGitRepositoryPath(".");      // "/path/to/projectName"
  if (strlen(git_repository_path) == 0) {
    free((void *) git_repository_path);
    printNonGitPrompt();
    return 0;
  }
  repo_status.repo_path   = git_repository_path;

  // if we can't create repo object, print default prompt
  git_repository *repo = NULL;
  if (git_repository_open(&repo, git_repository_path) != 0) {
    free((void *) git_repository_path);
    printNonGitPrompt();
    return 0;
  }

  // if we can't get ref to repo, it means we haven't committed anything yet.
  git_reference *head_ref = NULL;
  const git_oid *head_oid;
  if (git_repository_head(&head_ref, repo) != 0) {
    git_repository_free(repo);
    free((void *) git_repository_path);
    git_libgit2_shutdown();
    printNonGitPrompt();
    return 0;
  }
  head_oid = git_reference_target(head_ref);

  // get repo name and branch names
  repo_status.repo_name = strrchr(git_repository_path, '/') + 1; // "projectName"
  repo_status.branch_name = git_reference_shorthand(head_ref);
  char full_local_branch_name[128];
  sprintf(full_local_branch_name, "refs/heads/%s",  repo_status.branch_name);


  char full_remote_branch_name[128];
  sprintf(full_remote_branch_name, "refs/remotes/origin/%s", git_reference_shorthand(head_ref));

  // If there is no upstream ref, this is probably a stand-alone branch
  git_reference *upstream_ref = NULL;
  const git_oid *upstream_oid;
  if (git_reference_lookup(&upstream_ref, repo, full_remote_branch_name)) {
    git_reference_free(upstream_ref);
    repo_status.repo = NO_DATA;
  }
  else {
    upstream_oid = git_reference_target(upstream_ref);
    calculateAheadBehind(repo, head_oid, upstream_oid, &repo_status.ahead, &repo_status.behind);
  }
  
  // check if local and remote are the same
  if (repo_status.repo == UP_TO_DATE) {
    if (git_oid_cmp(head_oid, upstream_oid) != 0)
      repo_status.repo = MODIFIED;
  }


  // set up git status
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo, &opts) != 0) {
    git_reference_free(upstream_ref);
    git_reference_free(head_ref);
    git_repository_free(repo);
    free((void *) git_repository_path);
    git_libgit2_shutdown();
    printNonGitPrompt();
    return 0;
  }

  // check index and wt for diffs
  int status_count = git_status_list_entrycount(status_list);
  if (status_count != 0) {
    for (int i = 0; i < status_count; i++) {
      const git_status_entry *entry = git_status_byindex(status_list, i);
      if (entry->status == GIT_STATUS_CURRENT)         continue;

      if (entry->status & GIT_STATUS_INDEX_NEW)        repo_status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_MODIFIED)   repo_status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_RENAMED)    repo_status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_DELETED)    repo_status.index = MODIFIED;
      if (entry->status & GIT_STATUS_INDEX_TYPECHANGE) repo_status.index = MODIFIED;

      if (entry->status & GIT_STATUS_WT_RENAMED)       repo_status.wdir = MODIFIED;
      if (entry->status & GIT_STATUS_WT_DELETED)       repo_status.wdir = MODIFIED;
      if (entry->status & GIT_STATUS_WT_MODIFIED)      repo_status.wdir = MODIFIED;
      if (entry->status & GIT_STATUS_WT_TYPECHANGE)    repo_status.wdir = MODIFIED;
    }
  }

  // print the git prompt now that we have the info
  printGitPrompt(&repo_status);

  // clean up before we end
  git_status_list_free(status_list);
  git_reference_free(upstream_ref);
  git_reference_free(head_ref);
  git_repository_free(repo);
  free((void *) git_repository_path);
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
  const char *defaultPrompt = getenv("GP_DEFAULT_PROMPT") ?: "\\W $ ";
  printf("%s", defaultPrompt);
}

/* --------------------------------------------------
 *  When standing in a git repo, use this prompt.
 */
void printGitPrompt(const struct RepoStatus *repo_status) {

  // handle environment variables and default values
  const char* undigestedPrompt = getenv("GP_GIT_PROMPT") ?: "[\\pR/\\pL/\\pC]\n$ ";

  const char *colour[4];
  colour[ UP_TO_DATE  ] = getenv("GP_UP_TO_DATE") ?: "\033[0;32m";  // UP_TO_DATE - default green
  colour[ MODIFIED    ] = getenv("GP_MODIFIED")   ?: "\033[0;33m";  // MODIFIED   - default yellow
  colour[ NO_DATA     ] = getenv("GP_NO_DATA")    ?: "\033[0;37m";  // NO_DATA: default light grey
  colour[ RESET       ] = getenv("GP_RESET")      ?: "\033[0m";    // RESET      - RESET to default

  const char *wd_style = getenv("GP_GIT_WD_STYLE") ?: "basename";


  // handle WD field
  char cwd[2048]; // for output from getcwd
  char wd[2048];  // for storing the preferred working directory string style
  getcwd(cwd, sizeof(cwd));
  if (strcmp(wd_style, "cwd") == 0) {
    // show the entire path, from $HOME
    const char * home = getenv("HOME") ?: "";
    size_t common_length = strspn(cwd, home);
    sprintf(wd, "~/%s", cwd + common_length);
  }
  else if (strcmp(wd_style, "gitrelpath") == 0) { //todo when with_root is added, rename this to _no_root
    // show the entire path, from git-root (exclusive)
    size_t common_length = strspn(repo_status->repo_path, cwd);
    sprintf(wd, "%s", cwd + common_length);
    if (strlen(wd) == 0) {
      sprintf(wd, "//");
    }
  }
  /* else if (strcmp(wd_style, "relpath_with_root") == 0) { */
  /*   // show the entire path, from git-root (inclusive) */
  /*   size_t common_length = strspn(repo_status->repo_path, cwd); */
  /*   sprintf(wd, "%s", cwd + common_length); */
  /* } */
  else {
    // basename - show current directory only
    sprintf(wd, "%s", basename(cwd));
  }


  // look for and substitute escape codes
  char repo_temp[256]   = { '\0' };
  char branch_temp[256] = { '\0' };
  char cwd_temp[2048]   = { '\0' };
  char ab_temp[16]      = { '\0' }; // (ahead|-behind)
  char a_temp[4]        = { '\0' }; // ahead
  char b_temp[4]        = { '\0' }; // behind


  sprintf(repo_temp, "%s%s%s", colour[repo_status->repo], repo_status->repo_name, colour[RESET]);
  sprintf(branch_temp, "%s%s%s", colour[repo_status->index], repo_status->branch_name, colour[RESET]);
  sprintf(cwd_temp, "%s%s%s", colour[repo_status->wdir], wd, colour[RESET]);

  if (repo_status->ahead + repo_status->behind != 0)
    sprintf(ab_temp, "(%d,-%d)", repo_status->ahead, repo_status->behind);

  if (repo_status->ahead != 0)
    sprintf(a_temp, "%d", repo_status->ahead);

  if (repo_status->behind != 0)
    sprintf(b_temp, "%d", repo_status->behind);



  const char* searchStrings[] = { "\\pR", "\\pL", "\\pC", "\\pd", "\\pa", "\\pb" };
  const char* replaceStrings[] = { repo_temp, branch_temp, cwd_temp, ab_temp, a_temp, b_temp};

  char* prompt = strdup(undigestedPrompt);

  for (unsigned long i = 0; i < sizeof(searchStrings) / sizeof(searchStrings[0]); i++) {
    const char* searchString = searchStrings[i];
    const char* replaceString = replaceStrings[i];
    prompt = substitute(prompt, searchString, replaceString);
  }

  printf("%s", prompt);
  free(prompt);
}


/*
 * Calculate the number of commits ahead and behind the local branch is
 * compared to its upstream branch.
 * @return 0 on success, non-0 on error.
 */
int calculateAheadBehind(git_repository *repo,
                         const git_oid *local_oid,
                         const git_oid *upstream_oid,
                         int *ahead,
                         int *behind) {
  int aheadCount = 0;
  int behindCount = 0;
  git_oid id;


  // init walker
  git_revwalk *walker = NULL;
  if (git_revwalk_new(&walker, repo) != 0) {
    return -1;
  }

  // count number of commits ahead
  if (git_revwalk_push(walker, local_oid)    != 0 ||  // set where I want to start
      git_revwalk_hide(walker, upstream_oid) != 0) {  // set where the walk ends (exclusive)
    git_revwalk_free(walker);
    return -2;
  }
  while (git_revwalk_next(&id, walker) == 0) aheadCount++;


  // count number of commits behind
  git_revwalk_reset(walker);
  if (git_revwalk_push(walker, upstream_oid) != 0 || // set where I want to start          
      git_revwalk_hide(walker, local_oid)    != 0) { // set where the walk ends (exclusive)
    git_revwalk_free(walker);
    return -3;
  }
  while (git_revwalk_next(&id, walker) == 0) behindCount++;


  *ahead = aheadCount;
  *behind = behindCount;

  git_revwalk_free(walker);
  return 0;
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

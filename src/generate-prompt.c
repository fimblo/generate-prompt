/* --------------------------------------------------
 * Includes
 */
#include <stdio.h>
#include <string.h>
#include <git2.h>
#include <unistd.h>
#include <libgen.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>


/* --------------------------------------------------
 * Common global stuff
 */

// max buffer sizes
#define MAX_PATH_BUFFER_SIZE          2048
#define MAX_REPO_BUFFER_SIZE          256
#define MAX_BRANCH_BUFFER_SIZE        256
#define MAX_STYLE_BUFFER_SIZE         64
#define MAX_PARAM_MESSAGE_BUFFER_SIZE 64


enum states {
  RESET       = 0,
  NO_DATA     = 1,
  UP_TO_DATE  = 2,
  MODIFIED    = 3,
  CONFLICT    = 4,
};

enum exit_code {
  // success codes
  EXIT_GIT_PROMPT        =  0,
  EXIT_DEFAULT_PROMPT    =  1,
  EXIT_ABSENT_LOCAL_REF  =  2,

  // failure codes
  EXIT_FAIL_GIT_STATUS   = -1,
  EXIT_FAIL_REPO_OBJ     = -2,
};

// used to pass repo info around between functions
struct RepoContext {
  // Repo generics
  git_repository  *repo_obj;
  const char      *repo_name;
  const char      *repo_path;
  const char      *branch_name;
  git_reference   *head_ref;
  const git_oid   *head_oid;
  git_status_list *status_list;

  // Repo state
  int s_repo;
  int s_index;
  int s_wdir;
  int ahead;
  int behind;
  int conflict_count;
  int rebase_in_progress;

  // application stuff
  int exit_code;
};



/* --------------------------------------------------
 * Declarations
 * For detailed descriptions, see the function definitions further in
 * the code.
 */

// Prints default prompt for non-Git environments.
void printNonGitPrompt();

// Prints a Git-specific prompt with repo details.
void printGitPrompt(const struct RepoContext *repo_context);

// Finds the path to a Git repository from a given path.
const char *findGitRepositoryPath(const char *path);

// Calculates commit divergence between local and upstream branches.
int calculateDivergence(git_repository *repo,
                        const git_oid *local_oid,
                        const git_oid *upstream_oid,
                        int *ahead,
                        int *behind);

// Replaces all instances of 'search' with 'replacement' in 'text'.
char *substitute (const char *text, const char *search, const char *replacement);

// Initializes a RepoContext structure to default state.
void initializeRepoStatus(struct RepoContext *repo_context);

// Searches for and opens a git repository, updating RepoContext.
int findAndOpenGitRepository(struct RepoContext *repo_context);

// Releases resources tied to a RepoContext structure.
void cleanupResources(struct RepoContext *repo_context);

// Fetches head_ref and head_oid for a repository.
int getRepoHeadRef(struct RepoContext *repo_context);

// Extracts project and branch names from a RepoContext object.
void extractRepoAndBranchNames(struct RepoContext *repo_context);

// Determines statuses of repo elements relative to index and working directory.
void setupAndRetrieveGitStatus(struct RepoContext *repo_context);

// Checks if the repo is in the midst of an interactive rebase.
void checkForInteractiveRebase(struct RepoContext *repo_context);

// Identifies any conflicts/divergence between local and remote branches.
void checkForConflictsAndDivergence(struct RepoContext *repo_context);

// Function to display help message
void displayHelp(const char *message) {
  printf("USAGE\n");
  printf("  generate-prompt [-h|-H]\n");
  printf("\n");
  printf("OPTIONS\n");
  printf("  -h    This help message\n");
  printf("  -H    Show all configuration options\n");
  printf("\n");

  printf("OVERVIEW\n");
  printf("  Upon invocation, generate-prompt will print a context-aware prompt.\n");
  printf("  \n");
  printf("  There are two main modes:\n");
  printf("    - git mode\n");
  printf("    - normal mode\n");
  printf("  \n");
  printf("  If run while standing in a git repository, the prompt will output\n");
  printf("  git-specific information of your choosing.\n");
  printf("  \n");
  printf("  If run outside a git-repository, it will default to a normal prompt.\n");
  printf("\n");

  printf("CONFIGURATION\n");
  printf("  generate-prompt is configured entirely using environment variables.\n");
  printf("  You can find all configuration options by passing the '-H' parameter\n");
  printf("  to the binary.\n\n");

  if (message)
    printf("\nError: %s\n", message);
}

// Function to show configuration help
void displayConfigHelp() {
  printf("ENVIRONMENT VARIABLE OVERVIEW\n");
  printf("  GP_DEFAULT_PROMPT                for non-git dirs\n");
  printf("  GP_GIT_PROMPT                    for git dirs\n");
  printf("  GP_UP_TO_DATE                    up-to-date colour\n");
  printf("  GP_MODIFIED                      modified colour\n");
  printf("  GP_CONFLICT                      conflict colour\n");
  printf("  GP_NO_DATA                       no-data colour\n");
  printf("  GP_RESET                         reset terminal colour\n");
  printf("  GP_WD_STYLE                      style for \\pC instruction\n");
  printf("  GP_WD_STYLE_GITRELPATH_EXCLUSIVE how to show root if empty\n");
  printf("  GP_CONFLICT_STYLE                style for \\pK instruction\n");
  printf("  GP_REBASE_STYLE                  style for \\pi instruction\n");
  printf("  GP_A_DIVERGENCE_STYLE            style for \\pa instruction\n");
  printf("  GP_B_DIVERGENCE_STYLE            style for \\pb instruction\n");
  printf("  GP_AB_DIVERGENCE_STYLE           style for \\pd instruction\n");
  printf("\n\n");

  printf("INSTRUCTION OVERVIEW\n");
  printf("  \\pR     repo name (coloured)\n");
  printf("  \\pr     repo name\n");
  printf("  \\pL     branch name (coloured)\n");
  printf("  \\pl     branch name\n");
  printf("  \\pC     working directory (coloured)\n");
  printf("  \\pc     working directory\n");
  printf("\n");
  printf("  \\pa     ref divergence, ahead of upstream\n");
  printf("  \\pb     ref divergence, behind upstream\n");
  printf("  \\pd     ref divergence, ahead and behind\n");
  printf("\n");
  printf("  \\pi     show if interactive rebase\n");
  printf("  \\pK     show if conflict (coloured)\n");
  printf("  \\pk     show if conflict\n");
  printf("\n");
  printf("  \\pP     show prompt symbol $/# (coloured)\n");
  printf("  \\pp     show prompt symbol $/#\n");
  printf("\n\n");

  printf("EXAMPLE\n");
  printf("\n");
  printf("    export GP_GIT_PROMPT=\"\\pR/\\pL|\\pC \\pk\\pi\\n$ \"\n");
  printf("\n");
  printf("  Here we configure the GP_GIT_PROMPT so that it shows the following\n");
  printf("  information:\n");
  printf("  - The repository name*, followed by a slash\n");
  printf("  - The branch name*, followed by a pipe\n");
  printf("  - The current working directory*, followed by a space\n");
  printf("  - Add a note if the repo is currently in conflict\n");
  printf("  - Add a note if the repo is currently in interactive rebase\n");
  printf("  - Add a newline character\n");
  printf("  - Add a \"$ \" to position the cursor\n");
  printf("\n");
  printf("  * The repository name, the branch name, and the current working\n");
  printf("    directory will be coloured according to the state they are in.\n");
  printf("\n\n");

  printf("Please find extensive documentation on each of the above instructions\n");
  printf("and environment variables in the file README.org.\n");
}


/* --------------------------------------------------
 * Functions
 */
int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      displayHelp(NULL);
      return 0;
    }
    if (strcmp(argv[i], "-H") == 0) {
      displayConfigHelp();
      return 0;
    }
    else {
      char message[MAX_PARAM_MESSAGE_BUFFER_SIZE];
      sprintf(message, "Parameter '%s' unknown", argv[i]);
      displayHelp(message);
      return 1;
    }
  }

  struct RepoContext repo_context;
  git_libgit2_init();
  initializeRepoStatus(&repo_context);

  if(!findAndOpenGitRepository(&repo_context)) {
    printNonGitPrompt();
    git_libgit2_shutdown();
    return repo_context.exit_code;
  }

  if (!getRepoHeadRef(&repo_context)) {
    printNonGitPrompt();
    cleanupResources(&repo_context);
    return repo_context.exit_code;
  }

  extractRepoAndBranchNames(&repo_context);
  setupAndRetrieveGitStatus(&repo_context);
  checkForInteractiveRebase(&repo_context);
  checkForConflictsAndDivergence(&repo_context);

  printGitPrompt(&repo_context);

  cleanupResources(&repo_context);
  git_libgit2_shutdown();

  return EXIT_GIT_PROMPT;
}


/**
 * Searches for the path to a Git repository starting from the
 * specified path.
 *
 * @param path: Initial directory path to start the search from.
 *
 * @return Returns the path to the discovered Git repository or an
 *         empty string if no repository is found.
 */
const char *findGitRepositoryPath(const char *path) {
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


/**
 * Outputs a default command prompt when the user is not within a Git
 * repository or if there's an issue with the Git-specific prompt.
 */
void printNonGitPrompt() {
  const char *defaultPrompt = getenv("GP_DEFAULT_PROMPT") ?: "\\W $ ";
  printf("%s", defaultPrompt);
}


/**
 * Outputs a specialized command prompt tailored to provide
 * information about the current Git repository, such as repository
 * name, branch name, and various statuses.
 *
 * @param repo_context A structure containing details about the
 *                    repository's current status.
 */
void printGitPrompt(const struct RepoContext *repo_context) {

  // environment, else default values
  const char *undigestedPrompt = getenv("GP_GIT_PROMPT") ?: "[\\pR/\\pL/\\pC]\\pk\n$ ";
  const char *colour[5] = {
    [ RESET       ] = getenv("GP_RESET")      ?: "\\[\033[0m\\]",
    [ NO_DATA     ] = getenv("GP_NO_DATA")    ?: "\\[\033[0;37m\\]",
    [ UP_TO_DATE  ] = getenv("GP_UP_TO_DATE") ?: "\\[\033[0;32m\\]",
    [ MODIFIED    ] = getenv("GP_MODIFIED")   ?: "\\[\033[0;33m\\]",
    [ CONFLICT    ] = getenv("GP_CONFLICT")   ?: "\\[\033[0;31m\\]"
  };
  const char *wd_style            = getenv("GP_WD_STYLE")                      ?: "basename";
  const char *wd_relroot_pattern  = getenv("GP_WD_STYLE_GITRELPATH_EXCLUSIVE") ?: ":";
  const char *conflict_style      = getenv("GP_CONFLICT_STYLE")                ?: "(conflict: %d)";
  const char *rebase_style        = getenv("GP_REBASE_STYLE")                  ?: "(interactive rebase)";
  const char *a_divergence_style  = getenv("GP_A_DIVERGENCE_STYLE")            ?: "%d";
  const char *b_divergence_style  = getenv("GP_B_DIVERGENCE_STYLE")            ?: "%d";
  const char *ab_divergence_style = getenv("GP_AB_DIVERGENCE_STYLE")           ?: "(%d,-%d)";


  // handle working directory (wd) style
  char wd[MAX_PATH_BUFFER_SIZE];
  char full_path[MAX_PATH_BUFFER_SIZE];
  getcwd(full_path, sizeof(full_path));
  if (strcmp(wd_style, "basename") == 0) {
    // show basename of directory path
    sprintf(wd, "%s", basename(full_path));
  }
  else if (strcmp(wd_style, "cwd") == 0) {
    // show the entire path, from $HOME
    const char *home = getenv("HOME") ?: "";
    size_t common_length = strspn(full_path, home);
    sprintf(wd, "~/%s", full_path + common_length);
  }
  else if (strcmp(wd_style, "gitrelpath_exclusive") == 0) {
    // show the entire path, from git-root (exclusive)
    size_t common_length = strspn(repo_context->repo_path, full_path);
    if (common_length == strlen(full_path)) {
      sprintf(wd, "%s", wd_relroot_pattern);
    }
    else {
      sprintf(wd, "%s%s", wd_relroot_pattern, full_path + common_length + 1);
    }
  }
  else if (strcmp(wd_style, "gitrelpath_inclusive") == 0) {
    // show the entire path, from git-root (inclusive)
    size_t common_length = strspn(dirname((char *) repo_context->repo_path), full_path) + 1;
    sprintf(wd, "%s", full_path + common_length);
  }
  else {
    // if GP_WD_STYLE is set, but doesn't match any of the above
    // conditions, assume it can be safely added to the prompt. if it
    // isn't set, go with basename (set above)
    sprintf(wd, "%s", wd_style);
  }

  // handle interactive rebase style
  char rebase[MAX_STYLE_BUFFER_SIZE];
  if (repo_context->rebase_in_progress == 1) {
    sprintf(rebase, "%s", rebase_style);
  }
  else {
    rebase[0] = '\0';
  }


  // substitute base instructions
  char repo_colour[MAX_REPO_BUFFER_SIZE]     = { '\0' };
  char branch_colour[MAX_BRANCH_BUFFER_SIZE] = { '\0' };
  char cwd_colour[MAX_PATH_BUFFER_SIZE]      = { '\0' };
  sprintf(repo_colour,   "%s%s%s", colour[repo_context->s_repo],  repo_context->repo_name,   colour[RESET]);
  sprintf(branch_colour, "%s%s%s", colour[repo_context->s_index], repo_context->branch_name, colour[RESET]);
  sprintf(cwd_colour,    "%s%s%s", colour[repo_context->s_wdir],  wd,                        colour[RESET]);

  // prep for conflicts
  char conflict[MAX_STYLE_BUFFER_SIZE]        = { '\0' };
  char conflict_colour[MAX_STYLE_BUFFER_SIZE] = { '\0' };
  if (repo_context->conflict_count > 0) {
    sprintf(conflict, conflict_style, repo_context->conflict_count);
    sprintf(conflict_colour, "%s%s%s", colour[CONFLICT], conflict, colour[RESET]);
  }

  // prep for commit divergence
  char divergence_ab[MAX_STYLE_BUFFER_SIZE] = { '\0' };
  char divergence_a[MAX_STYLE_BUFFER_SIZE]  = { '\0' };
  char divergence_b[MAX_STYLE_BUFFER_SIZE]  = { '\0' };
  if (repo_context->ahead + repo_context->behind > 0)
    sprintf(divergence_ab, ab_divergence_style, repo_context->ahead, repo_context->behind);
  if (repo_context->ahead != 0)
    sprintf(divergence_a, a_divergence_style, repo_context->ahead);
  if (repo_context->behind != 0)
    sprintf(divergence_b, b_divergence_style, repo_context->behind);

  // prep for showing user-class dependent symbol
  char show_prompt_colour[MAX_STYLE_BUFFER_SIZE] = { '\0'};
  char show_prompt[MAX_STYLE_BUFFER_SIZE]        = { '\0'};
  const char * prompt_symbol;

  // This is a very ugly solution. My problem is this. I'd like to
  // only use getuid() to determine the user type, but I can't mock C
  // functions from a bash test suite. Well... I could do a
  // conditional compilation with an #ifdef to pass the uid.. but then
  // I would need to compile two versions for my tests. Ugly as well,
  // but in another way.
  //
  // So instead, I figure that if someone is courageous enough to run
  // my binary as root, they should never get the '$" symbol in their
  // prompt. On the other hand, if you don't have escalated rights,
  // perhaps it's not quite as dangerous to get the '#' symbol in your
  // prompt - since having that symbol won't get you escalated rights
  // to your system.
  const char *username = getenv("USER");
  if (getuid() == 0) {
    prompt_symbol = "#";
  } else if (username && strcmp(username, "root") == 0) {
      prompt_symbol = "#";
  }
  else {
    prompt_symbol = "$";
  }
  sprintf(show_prompt_colour, "%s%s%s", colour[repo_context->s_wdir], prompt_symbol, colour[RESET]);
  sprintf(show_prompt, "%s", prompt_symbol);

  // apply all instructions found
  const char *instructions[][2] = {
    { "\\pR", repo_colour              },
    { "\\pr", repo_context->repo_name  },

    { "\\pL", branch_colour            },
    { "\\pl", repo_context->branch_name },

    { "\\pC", cwd_colour               },
    { "\\pc", wd                       },

    { "\\pK", conflict_colour          },
    { "\\pk", conflict                 },

    { "\\pd", divergence_ab            },
    { "\\pa", divergence_a             },
    { "\\pb", divergence_b             },

    { "\\pi", rebase                   },

    { "\\pP", show_prompt_colour       },
    { "\\pp", show_prompt              },
  };


  char *prompt = strdup(undigestedPrompt);
  for (unsigned long i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
    prompt = substitute(prompt, instructions[i][0], instructions[i][1]);
  }

  printf("%s", prompt);
  free(prompt);
}


/**
 * Calculates the commit divergence between a local branch and its
 * upstream counterpart. It provides information about how many
 * commits the local branch is ahead or behind the upstream.
 *
 * @param repo        Pointer to the Git repository in context.
 * @param local_oid   OID (Object ID) of the local branch's latest
 *                    commit.
 * @param upstream_oid OID of the upstream branch's latest commit.
 * @param ahead       Output parameter where the number of commits
 *                    the local branch is ahead of upstream will be
 *                    stored.
 * @param behind      Output parameter where the number of commits
 *                    the local branch is behind the upstream will be
 *                    stored.
 *                    
 * @return Returns 0 on successful calculation, otherwise returns a
 *         non-zero error code.
 */
int calculateDivergence(git_repository *repo,
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


/**
 * Helper function that performs a string substitution operation.
 * It searches for occurrences of a 'search' string within a 'text'
 * string and replaces them with a 'replacement' string.
 *
 * @param text        The original string where substitutions should
 *                    be made.
 * @param search      The substring to look for within the 'text'.
 * @param replacement The string to replace 'search' with.
 * @return Returns a new string with all instances of 'search'
 *         replaced by 'replacement'.
 */
char *substitute(const char *text, const char *search, const char *replacement) {
  char *message = strdup(text);
  char *found = strstr(message, search);

  while (found) {
    size_t prefix_length = found - message;
    size_t suffix_length = strlen(found + strlen(search));

    size_t new_length = prefix_length + strlen(replacement) + suffix_length + 1;
    char *temp = malloc(new_length);  // Allocate temporary buffer for the new string

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


/**
 * Initializes the given RepoContext object to its default state. The
 * RepoContext structure is utilized to share repository-related state
 * information among various functions.
 * 
 * @param repo_context: Pointer to the RepoContext structure to be
 *                     initialized.
 */
void initializeRepoStatus(struct RepoContext *repo_context) {
  repo_context->repo_obj           = NULL;
  repo_context->repo_name          = NULL;
  repo_context->repo_path          = NULL;
  repo_context->branch_name        = NULL;
  repo_context->head_ref           = NULL;
  repo_context->head_oid           = NULL;
  repo_context->status_list        = NULL;
  repo_context->s_repo             = UP_TO_DATE;
  repo_context->s_index            = UP_TO_DATE;
  repo_context->s_wdir             = UP_TO_DATE;
  repo_context->ahead              = 0;
  repo_context->behind             = 0;
  repo_context->conflict_count     = 0;
  repo_context->rebase_in_progress = 0;
  repo_context->exit_code          = 0;
}


/**
 * Searches upward through the directory tree from the current
 * directory ("."), attempting to locate a git repository. If found,
 * populates the RepoContext structure with the repository and its
 * path.
 *
 * @param repo_context: Pointer to a RepoContext structure to be
 *                      populated if a repo is found.
 * @return Returns 1 if a repository is found, otherwise returns 0.
 */
int findAndOpenGitRepository(struct RepoContext *repo_context) {
  const char *git_repository_path = findGitRepositoryPath(".");
  if (strlen(git_repository_path) == 0) {
    free((void *) git_repository_path);
    repo_context->exit_code = EXIT_DEFAULT_PROMPT;
    return 0;
  }
  repo_context->repo_path = git_repository_path;  // "/path/to/projectName"

  git_repository *repo = NULL;
  if (git_repository_open(&repo, git_repository_path) != 0) {
    free((void *) git_repository_path);
    git_repository_free(repo);
    repo_context->exit_code = EXIT_FAIL_REPO_OBJ;
    return 0;
  }

  repo_context->repo_obj = repo;
  return 1;
}


/**
 * Frees allocated resources associated with the given RepoContext.
 * This function should be called before exiting the program to ensure 
 * that memory and other resources are properly released.
 *
 * @param repo_context: Pointer to a RepoContext structure whose
 *                     resources will be freed.
 */
void cleanupResources(struct RepoContext *repo_context) {
  if (repo_context->repo_obj) {
    git_repository_free(repo_context->repo_obj);
    repo_context->repo_obj = NULL;
  }
  if (repo_context->head_ref) {
    git_reference_free(repo_context->head_ref);
    repo_context->head_ref = NULL;
  }
  if (repo_context->status_list) {
    git_status_list_free(repo_context->status_list);
    repo_context->status_list = NULL;
  }
  if (repo_context->repo_path) {
    free((void *) repo_context->repo_path);
    repo_context->repo_path = NULL;
  }
  git_libgit2_shutdown();
}


/**
 * Attempts to acquire the head_ref and head_oid of the specified repo.
 *
 * @param repo_context: Pointer to a RepoContext structure where the
 *                     head_ref and head_oid will be stored if found.
 * @return Returns 1 if successful in acquiring the references, and 0
 *                     otherwise.
 */
int getRepoHeadRef(struct RepoContext *repo_context) {
  git_reference *head_ref = NULL;
  const git_oid *head_oid;
  if (git_repository_head(&head_ref, repo_context->repo_obj) != 0) {
    repo_context->exit_code = EXIT_ABSENT_LOCAL_REF;
    return 0;
  }
  head_oid = git_reference_target(head_ref);

  repo_context->head_ref = head_ref;
  repo_context->head_oid = head_oid;
  return 1;
}


/**
 * Extracts the name of the git project and the current branch from
 * the provided RepoContext object.
 *
 * @param repo_context: Pointer to the RepoContext structure. After the
 *                     function call, this structure will hold the
 *                     extracted git project name and the current
 *                     branch name.
 */
void extractRepoAndBranchNames(struct RepoContext *repo_context) {
  repo_context->repo_name = strrchr(repo_context->repo_path, '/') + 1;
  repo_context->branch_name = git_reference_shorthand(repo_context->head_ref);
}


/**
 * Iterates through each element in the repo to determine their status
 * relative to the index and working directory. Also tallies any
 * conflicts.
 *
 * @param repo_context: Pointer to the RepoContext structure. Upon
 *                     completion, this structure will reflect the
 *                     working directory, index, and conflict
 *                     statuses.
 */
void setupAndRetrieveGitStatus(struct RepoContext *repo_context) {
  // Suppressing this warning due to a known issue with
  // GIT_STATUS_OPTIONS_INIT not initializing all fields. We're
  // manually setting the necessary fields afterwards.
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  #pragma GCC diagnostic pop
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo_context->repo_obj, &opts) != 0) {
    git_reference_free(repo_context->head_ref);
    git_repository_free(repo_context->repo_obj);
    free((void *) repo_context->repo_path);
    repo_context->exit_code = EXIT_FAIL_GIT_STATUS;
    return;
  }

  int status_count = git_status_list_entrycount(status_list);
  for (int i = 0; i < status_count; i++) {
    const git_status_entry *entry = git_status_byindex(status_list, i);
    if (entry->status == GIT_STATUS_CURRENT) continue;
    if (entry->status & GIT_STATUS_CONFLICTED)       repo_context->conflict_count++;

    if (entry->status & GIT_STATUS_INDEX_NEW)        repo_context->s_index = MODIFIED;
    if (entry->status & GIT_STATUS_INDEX_MODIFIED)   repo_context->s_index = MODIFIED;
    if (entry->status & GIT_STATUS_INDEX_RENAMED)    repo_context->s_index = MODIFIED;
    if (entry->status & GIT_STATUS_INDEX_DELETED)    repo_context->s_index = MODIFIED;
    if (entry->status & GIT_STATUS_INDEX_TYPECHANGE) repo_context->s_index = MODIFIED;

    if (entry->status & GIT_STATUS_WT_RENAMED)       repo_context->s_wdir = MODIFIED;
    if (entry->status & GIT_STATUS_WT_DELETED)       repo_context->s_wdir = MODIFIED;
    if (entry->status & GIT_STATUS_WT_MODIFIED)      repo_context->s_wdir = MODIFIED;
    if (entry->status & GIT_STATUS_WT_TYPECHANGE)    repo_context->s_wdir = MODIFIED;
  }

  repo_context->status_list = status_list;
}


/**
 * Checks if the current repository is in the middle of an interactive
 * rebase operation by looking for the presence of 'rebase-merge' or
 * 'rebase-apply' directories in the '.git' folder.
 *
 * @param repo_context: Pointer to the RepoContext structure. The
 *                     'rebase_in_progress' flag will be set to 1 if
 *                     an interactive rebase is in progress.
 */
void checkForInteractiveRebase(struct RepoContext *repo_context) {
  char rebaseMergePath[MAX_PATH_BUFFER_SIZE];
  char rebaseApplyPath[MAX_PATH_BUFFER_SIZE];
  snprintf(rebaseMergePath, sizeof(rebaseMergePath), "%s/.git/rebase-merge", repo_context->repo_path);
  snprintf(rebaseApplyPath, sizeof(rebaseApplyPath), "%s/.git/rebase-apply", repo_context->repo_path);

  struct stat mergeStat, applyStat;
  if (stat(rebaseMergePath, &mergeStat) == 0 || stat(rebaseApplyPath, &applyStat) == 0) {
    repo_context->rebase_in_progress = 1;
  }
}


/**
 * Inspects the repository to detect if there are any conflicts or
 * divergence between the local and remote branches. It sets the
 * 's_repo' state of the RepoContext structure based on the findings
 * (e.g., CONFLICT, NO_DATA, UP_TO_DATE, MODIFIED).
 *
 * @param repo_context: Pointer to the RepoContext structure. This will
 *                     be updated with conflict and divergence
 *                     information.
 */
void checkForConflictsAndDivergence(struct RepoContext *repo_context) {
  if (repo_context->conflict_count != 0) {
    // If we're in conflict, mark the repo state accordingly.
    repo_context->s_repo = CONFLICT;
  }
  else {
    char full_remote_branch_name[128];
    sprintf(full_remote_branch_name, "refs/remotes/origin/%s", git_reference_shorthand(repo_context->head_ref));

    // If there is no upstream ref, this is probably a stand-alone branch
    git_reference *upstream_ref = NULL;
    const git_oid *upstream_oid;

    const int retval = git_reference_lookup(&upstream_ref, repo_context->repo_obj, full_remote_branch_name);
    if (retval != 0) {
      git_reference_free(upstream_ref);
      repo_context->s_repo = NO_DATA;
    }
    else {
      upstream_oid = git_reference_target(upstream_ref);

      // if the upstream_oid is null, we can't get the divergence, so
      // might as well set it to NO_DATA. Oh and btw, when there's no
      // conflict _and_ upstream_OID is NULL, then it seems we're
      // inside of an interactive rebase - when it's not useful to
      // check for divergences anyway.
      if (upstream_oid == NULL) {
        repo_context->s_repo = NO_DATA;
      }
      else {
        calculateDivergence(repo_context->repo_obj,
                            repo_context->head_oid,
                            upstream_oid,
                            &repo_context->ahead,
                            &repo_context->behind);
      }

    }

    // check if local and remote are the same
    if (repo_context->s_repo == UP_TO_DATE) {
      if (git_oid_cmp(repo_context->head_oid, upstream_oid) != 0)
        repo_context->s_repo = MODIFIED;
    }

    git_reference_free(upstream_ref);
  }
}

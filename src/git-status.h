/*
  header file for git-status.c
*/
#include <git2.h>

// max buffer sizes
#define MAX_PATH_BUFFER_SIZE          2048
#define MAX_REPO_BUFFER_SIZE          256
#define MAX_BRANCH_BUFFER_SIZE        256
#define MAX_STYLE_BUFFER_SIZE         64
#define MAX_PARAM_MESSAGE_BUFFER_SIZE 64

enum exit_code {
  // success codes
  EXIT_GIT_PROMPT        =  0,
  EXIT_DEFAULT_PROMPT    =  1,
  EXIT_ABSENT_LOCAL_REF  =  2,

  // failure codes
  EXIT_FAIL_GIT_STATUS   = -1,
  EXIT_FAIL_REPO_OBJ     = -2,
};

struct RepoContext {
  git_repository  *repo_obj;
  const char      *repo_name;
  const char      *repo_path;
  const char      *branch_name;
  git_reference   *head_ref;
  const git_oid   *head_oid;
  git_status_list *status_list;
};

void initializeRepoContext(struct RepoContext *repo_context);
int populateRepoContext(struct RepoContext *context, const char *path);
const char * getRepoName(struct RepoContext *context);
const char * getBranchName(struct RepoContext *context);
 

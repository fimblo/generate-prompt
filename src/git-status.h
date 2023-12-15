#ifndef GITSTATUS_H
#define GITSTATUS_H
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


enum states {
  NO_DATA,
  UP_TO_DATE,
  MODIFIED,
  ENUM_SIZE
};

extern const char *state_names[ENUM_SIZE];

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

struct RepoStatus {
  // Repo state
  int status_repo;
  int status_staged;
  int status_unstaged;
  int ahead;
  int behind;
  int conflict_count;
  int rebase_in_progress;
  int staged_changes_num;
  int unstaged_changes_num;
};

void initializeRepoContext(struct RepoContext *repo_context);
void initializeRepoStatus(struct RepoStatus *repo_status);
int populateRepoContext(struct RepoContext *context, const char *path);
const char * getRepoName(struct RepoContext *context);
const char * getBranchName(struct RepoContext *context);
git_status_list * getRepoStatusList(struct RepoContext *context); 
void getRepoStatus(git_status_list * status_list, struct RepoStatus *status);




#endif //GITSTATUS_H

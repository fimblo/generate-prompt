/*
  git-status.c

  Assortment of functions to get git status
*/
#include <string.h>
#include <stdlib.h>
#include <git2.h>
#include "git-status.h"

const char *state_names[ENUM_SIZE] = {
    "NO_DATA",
    "UP_TO_DATE",
    "MODIFIED"
};


void initializeRepoContext(struct RepoContext *context) {
  context->repo_obj     = NULL;
  context->repo_name    = NULL;
  context->repo_path    = NULL;
  context->branch_name  = NULL;
  context->head_ref     = NULL;
  context->head_oid     = NULL;
  context->status_list  = NULL;
}

void initializeRepoStatus(struct RepoStatus *status) {
  status->status_repo          = UP_TO_DATE;
  status->ahead                = 0;
  status->behind               = 0;

  status->status_staged        = UP_TO_DATE;
  status->staged_changes_num   = 0;

  status->status_unstaged      = UP_TO_DATE;
  status->unstaged_changes_num = 0;

  status->conflict_num         = 0;
  status->rebase_in_progress   = 0;
}


// helper function. not exported
const char *__findGitRepositoryPath(const char *path) {
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

    return __findGitRepositoryPath(parent_path);
  }
  else {
    return strdup("");
  }
}


int populateRepoContext(struct RepoContext *context, const char *path) {
  const char *git_repository_path;
  git_repository *repo     = NULL;
  git_reference *head_ref  = NULL;
  const git_oid * head_oid = NULL;

  git_repository_path = __findGitRepositoryPath(path);
  if (strlen(git_repository_path) == 0) {
    free((void *) git_repository_path);
    return EXIT_DEFAULT_PROMPT;
  }

  if (git_repository_open(&repo, git_repository_path) != 0) {
    free((void *) git_repository_path);
    git_repository_free(repo);
    return EXIT_FAIL_REPO_OBJ;
  }

  if (git_repository_head(&head_ref, repo) != 0) {
    return EXIT_ABSENT_LOCAL_REF;
  }
  head_oid = git_reference_target(head_ref);

  context->repo_path = git_repository_path;  // "/path/to/projectName"
  context->repo_obj  = repo;
  context->head_ref  = head_ref;
  context->head_oid  = head_oid;
  return 1;
}

const char * getRepoName(struct RepoContext *context) {
  context->repo_name = strrchr(context->repo_path, '/') + 1;
  return context->repo_name;
}

const char * getBranchName(struct RepoContext *context) {
  context->branch_name = git_reference_shorthand(context->head_ref);
  return context->branch_name;
}

git_status_list * getRepoStatusList(struct RepoContext * context) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
#pragma GCC diagnostic pop

  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, context->repo_obj, &opts) != 0) {
    git_reference_free(context->head_ref);
    git_repository_free(context->repo_obj);
    free((void *) context->repo_path);
    return NULL;
  }
  context->status_list = status_list;
  return status_list;
}

void getRepoStatus(git_status_list * status_list, struct RepoStatus *status) {
  int staged_changes   = 0;
  int unstaged_changes = 0;

  int status_count = git_status_list_entrycount(status_list);
  for (int i = 0; i < status_count; i++) {
    const git_status_entry *entry = git_status_byindex(status_list, i);
    if (entry->status == GIT_STATUS_CURRENT) continue;

    // Check for conflicts
    if (entry->status & GIT_STATUS_CONFLICTED)
      status->conflict_num++;

    // Check for staged changes
    if (entry->status & (GIT_STATUS_INDEX_NEW      |
                         GIT_STATUS_INDEX_MODIFIED |
                         GIT_STATUS_INDEX_RENAMED  |
                         GIT_STATUS_INDEX_DELETED  |
                         GIT_STATUS_INDEX_TYPECHANGE)) {
      status->status_staged = MODIFIED;
      staged_changes++;
    }

    // Check for unstaged changes
    if (entry->status & (GIT_STATUS_WT_MODIFIED |
                         GIT_STATUS_WT_DELETED  |
                         GIT_STATUS_WT_RENAMED  |
                         GIT_STATUS_WT_TYPECHANGE)) {
      status->status_unstaged = MODIFIED;
      unstaged_changes++;
    }
  }

  status->staged_changes_num = staged_changes;
  status->unstaged_changes_num = unstaged_changes;
}

/*
  git-status.c

  Assortment of functions to get git status
*/
#include <string.h>
#include <stdlib.h>
#include <git2.h>
#include "git-status.h"



void initializeRepoContext(struct RepoContext *context) {
  context->repo_obj     = NULL;
  context->repo_name    = NULL;
  context->repo_path    = NULL;
  context->branch_name  = NULL;
  context->head_ref     = NULL;
  context->head_oid     = NULL;
  context->status_list  = NULL;
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

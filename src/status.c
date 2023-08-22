/* --------------------------------------------------
 * Includes
 */
#include <stdio.h>
#include <string.h>
#include <git2.h>


/* --------------------------------------------------
 * Enums and colors
 */


/* --------------------------------------------------
 * Declarations
 */
const char* findGitRepositoryPath(const char *path);
static void print_long(git_status_list *status);


/* --------------------------------------------------
 * Functions
 */
int main() {
  git_libgit2_init();

  // get path to git repo at '.' else print default prompt
  const char *git_repository_path = findGitRepositoryPath(".");      // "/path/to/projectName"
  if (strlen(git_repository_path) == 0) {
    printf("Not in git repo\n");
    return 0;
  }

  // if we can't create repo object, print default prompt
  git_repository *repo = NULL;
  if (git_repository_open(&repo, git_repository_path) != 0) {
    printf("Can't create repo object\n");
    return 0;
  }

  // if we can't get ref to repo, print defualt prompt
  git_reference *head_ref = NULL;
  if (git_repository_head(&head_ref, repo) != 0) {
    printf("Can't get ref to repo\n");
    git_repository_free(repo);
    return 1;
  }

  // get repo name and branch name
  const char *repo_name = strrchr(git_repository_path, '/') + 1; // "projectName"
  const char *branch_name = git_reference_shorthand(head_ref);


  // set up git status
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  opts.flags =
    GIT_STATUS_OPT_INCLUDE_UNTRACKED |
    GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
    GIT_STATUS_OPT_SORT_CASE_SENSITIVELY |
    GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS |
    GIT_STATUS_OPT_INCLUDE_IGNORED;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo, &opts) != 0) {
    git_reference_free(head_ref);
    git_repository_free(repo);
    printf("Can't get git repo status\n");
    return 1;
  }

  print_long(status_list);


  git_status_list_free(status_list);
  git_reference_free(head_ref);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
}


// ripped from libgit2/src/libgit2/status.c
static void print_long(git_status_list *status) {
  size_t i, maxi = git_status_list_entrycount(status);
  const git_status_entry *s;
  //  int rm_in_workdir = 0;
  const char *old_path, *new_path;

  for (i = 0; i < maxi; ++i) {
    char *istatus = NULL;

    s = git_status_byindex(status, i);

    if (s->status == GIT_STATUS_CURRENT)
      continue;
    /* if (s->status & GIT_STATUS_WT_DELETED) */
    /*   rm_in_workdir = 1; */
    if (s->status & GIT_STATUS_INDEX_NEW)
      istatus = "I new file: ";
    if (s->status & GIT_STATUS_INDEX_MODIFIED)
      istatus = "I modified: ";
    if (s->status & GIT_STATUS_INDEX_DELETED)
      istatus = "I deleted:  ";
    if (s->status & GIT_STATUS_INDEX_RENAMED)
      istatus = "I renamed:  ";
    if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
      istatus = "I typechange:";

    if (istatus == NULL)
      continue;


    // changes to be committed
    old_path = s->head_to_index->old_file.path;
    new_path = s->head_to_index->new_file.path;

    if (old_path && new_path && strcmp(old_path, new_path))
      printf("%s (staged) %s -> %s\n", istatus, old_path, new_path);
    else
      printf("%s (staged) %s\n", istatus, old_path ? old_path : new_path);
  }

  for (i = 0; i < maxi; ++i) {
    char *wstatus = NULL;

    s = git_status_byindex(status, i);

    if (s->status == GIT_STATUS_CURRENT || s->index_to_workdir == NULL)
      continue;


    if (s->status & GIT_STATUS_WT_MODIFIED)
      wstatus = "W modified: ";
    if (s->status & GIT_STATUS_WT_DELETED)
      wstatus = "W deleted:  ";
    if (s->status & GIT_STATUS_WT_RENAMED)
      wstatus = "W renamed:  ";
    if (s->status & GIT_STATUS_WT_TYPECHANGE) {
      wstatus = "W typechange:";
      printf("typechange");
    }

    if (wstatus == NULL)
      continue;


    old_path = s->index_to_workdir->old_file.path;
    new_path = s->index_to_workdir->new_file.path;

    if (old_path && new_path && strcmp(old_path, new_path))
      printf("#%s  %s -> %s\n", wstatus, old_path, new_path);
    else
      printf("%s  %s\n", wstatus, old_path ? old_path : new_path);
  }

  for (i = 0; i < maxi; ++i) {
    s = git_status_byindex(status, i);

    if (s->status == GIT_STATUS_WT_NEW) {
      // untracked files
      printf("W untracked: %s\n", s->index_to_workdir->old_file.path);
    }
  }

  for (i = 0; i < maxi; ++i) {
    s = git_status_byindex(status, i);

    //    if (s->status == GIT_STATUS_IGNORED) {
      // ignored files
      //printf("W ignored files: %s\n", s->index_to_workdir->old_file.path);
    //    }
  }
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

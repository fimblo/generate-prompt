#include <stdio.h>
#include <git2.h>

#define COLOR_GIT_CLEAN    "\033[1;30m"
#define COLOR_GIT_MODIFIED "\033[0;33m"
#define COLOR_GIT_STAGED   "\033[0;36m"
#define COLOR_RESET        "\033[0m"

int main() {
  git_libgit2_init();

  git_repository *repo = NULL;
  if (git_repository_open(&repo, ".") != 0) {
    return 0;
  }

  git_reference *head_ref = NULL;
  if (git_repository_head(&head_ref, repo) != 0) {
    git_repository_free(repo);
    return 1;
  }

  const char *branch_name = git_reference_shorthand(head_ref);

  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

  git_status_list *status_list = NULL;
  if (git_status_list_new(&status_list, repo, &opts) != 0) {
    git_reference_free(head_ref);
    git_repository_free(repo);
    return 1;
  }

  int status_count = git_status_list_entrycount(status_list);
  if (status_count == 0) {
    printf("%s%s%s", COLOR_GIT_CLEAN, branch_name, COLOR_RESET);
  } else {
    int i;
    int staged_changes = 0;
    int unstaged_changes = 0;
    for (i = 0; i < status_count; i++) {
      const git_status_entry *entry = git_status_byindex(status_list, i);
      if (entry->status == GIT_STATUS_INDEX_NEW || entry->status == GIT_STATUS_INDEX_MODIFIED) {
        staged_changes = 1;
      }
      if (entry->status == GIT_STATUS_WT_MODIFIED) {
        unstaged_changes = 1;
      }
    }
    if (staged_changes) {
      printf("%s%s%s", COLOR_GIT_STAGED, branch_name, COLOR_RESET);
    } else if (unstaged_changes) {
      printf("%s%s*%s", COLOR_GIT_MODIFIED, branch_name, COLOR_RESET);
    }
  }

  git_status_list_free(status_list);
  git_reference_free(head_ref);
  git_repository_free(repo);
  git_libgit2_shutdown();

  printf("Branch: %s", branch_name);
  return 0;
}

#include <stdio.h>
#include <git2.h>
#include "git-status.h"

//int main(int argc, char *argv[]) {
int main(void) {
  git_libgit2_init();
  struct RepoContext context;
  initializeRepoContext(&context);
  populateRepoContext(&context, ".");

  printf("Repo name:   %s\n", getRepoName(&context));
  printf("Branch name: %s\n", getBranchName(&context));
  
  git_libgit2_shutdown();
}

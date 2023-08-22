#include <git2.h>
#include <stdio.h>
#include <string.h>

int main() {
    git_libgit2_init();

    git_repository *repo = NULL;
    git_repository_open(&repo, ".");

    // Get the current branch name
    const char *your_branch_name = NULL;
    git_reference *head_ref = NULL;
    git_repository_head(&head_ref, repo);
    your_branch_name = git_reference_shorthand(head_ref);

    // Get the remote tracking branch name (e.g., origin/main)
    const char *remote_tracking_branch_name = NULL;
    git_reference *upstream_ref = NULL;
    git_reference_lookup(&upstream_ref, repo, "HEAD");
    const git_oid *upstream_target = git_reference_target(upstream_ref);
    remote_tracking_branch_name = git_oid_tostr_s(upstream_target) + strlen("refs/remotes/");

    git_reference *your_branch_ref = NULL;
    git_reference_lookup(&your_branch_ref, repo, your_branch_name);

    git_reference *remote_branch_ref = NULL;
    git_reference_lookup(&remote_branch_ref, repo, remote_tracking_branch_name);

    int cmp_result = git_reference_cmp(your_branch_ref, remote_branch_ref);

    if (cmp_result != 0) {
        printf("Your branch is different from the remote tracking branch.\n");
    } else {
        printf("Your branch is the same as the remote tracking branch.\n");
    }

    git_reference_free(head_ref);
    git_reference_free(upstream_ref);
    git_reference_free(your_branch_ref);
    git_reference_free(remote_branch_ref);
    git_repository_free(repo);

    git_libgit2_shutdown();

    return 0;
}

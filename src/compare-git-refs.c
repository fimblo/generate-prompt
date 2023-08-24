#include <git2.h>
#include <stdio.h>
#include <string.h>

int main() {
    git_libgit2_init();

    git_repository *repo = NULL;
    int error = git_repository_open(&repo, ".");
    if (error != 0) {
        printf("Failed to open repository.\n");
        git_libgit2_shutdown();
        return 1;
    }

    // Local branch stuff //////////////////////////////////////////////////

    git_reference *head_ref = NULL;
    error = git_repository_head(&head_ref, repo);
    if (error != 0) {
        printf("Failed to get HEAD reference.\n");
        git_repository_free(repo);
        git_libgit2_shutdown();
        return 1;
    }

    char local_branch_name[128];
    sprintf(local_branch_name, "refs/heads/%s",  git_reference_shorthand(head_ref));

    
    git_reference *local_branch_ref = NULL;
    error = git_reference_lookup(&local_branch_ref, repo, local_branch_name);
    if (error != 0) {
        printf("Failed to lookup local branch reference.\n");
        git_reference_free(head_ref);
        git_repository_free(repo);
        git_libgit2_shutdown();
        return 1;
    }

    const git_oid *local_branch_commit_id = git_reference_target(local_branch_ref);



    // Upstream branch stuff //////////////////////////////////////////////////
    char full_remote_branch_name[128];
    sprintf(full_remote_branch_name, "refs/remotes/origin/%s", git_reference_shorthand(head_ref));


    git_reference *upstream_ref = NULL;
    error = git_reference_lookup(&upstream_ref, repo, full_remote_branch_name); 
    if (error != 0) {
        printf("Failed to lookup upstream reference.\n");
        git_reference_free(local_branch_ref);
        git_reference_free(head_ref);
        git_repository_free(repo);
        git_libgit2_shutdown();
        return 1;
    }

    const char *short_remote_branch_name = git_reference_shorthand(upstream_ref);

    char remote_branch_name[128];
    sprintf(remote_branch_name, "refs/remotes/%s", short_remote_branch_name);



    git_reference *remote_branch_ref = NULL;
    error = git_reference_lookup(&remote_branch_ref, repo, remote_branch_name);
    if (error != 0) {
        printf("Failed to lookup remote branch reference.\n");
        git_reference_free(local_branch_ref);
        git_reference_free(upstream_ref);
        git_reference_free(head_ref);
        git_repository_free(repo);
        git_libgit2_shutdown();
        return 1;
    }

    const git_oid *remote_branch_commit_id = git_reference_target(remote_branch_ref);


    int cmp_result = git_oid_cmp(local_branch_commit_id, remote_branch_commit_id);


    printf("Local commit:  '%s'\tName: '%s'\n",
           git_oid_tostr_s(local_branch_commit_id),
           local_branch_name);
    printf("Remote commit: '%s'\tName: '%s'\n",
           git_oid_tostr_s(remote_branch_commit_id),
           remote_branch_name);
    printf("\n");


    if (cmp_result != 0) {
        printf("Local branch is different from the remote tracking branch.\n");
    } else {
        printf("Local branch is the same as the remote tracking branch.\n");
    }

    git_reference_free(local_branch_ref);
    git_reference_free(remote_branch_ref);
    git_reference_free(upstream_ref);
    git_reference_free(head_ref);
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}

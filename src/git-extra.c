#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

int read_first_line(const char *path, char *buffer, size_t size) {
    FILE *file = fopen(path, "r");
    if (file == NULL) return 1;
    fgets(buffer, size, file);
    fclose(file);
    buffer[strcspn(buffer, "\n")] = 0;// Remove newline if exists
    return 0;
}


int git_head_status(const char *vcs_specific_dir,
                    char **vcs_head_status,
                    char **vcs_head_details) {
    char filepath[256];
    char step[20] = {0}, total[20] = {0};
    
    // Initialize
    *vcs_head_status = NULL;
    *vcs_head_details = NULL;

    // MERGE_HEAD
    snprintf(filepath, sizeof(filepath), "%s/MERGE_HEAD", vcs_specific_dir);
    if (file_exists(filepath)) {
        *vcs_head_status = strdup("MERGING");
        return 0;
    }
  
    // REBASE-MERGE
    snprintf(filepath, sizeof(filepath), "%s/rebase-merge", vcs_specific_dir);
    if (file_exists(filepath)) {
        read_first_line("rebase-merge/msgnum", step, sizeof(step));
        read_first_line("rebase-merge/end", total, sizeof(total));
        
        snprintf(filepath, sizeof(filepath), "%s/rebase-merge/interactive", vcs_specific_dir);
        *vcs_head_status = file_exists(filepath) ? strdup("REBASE-i") : strdup("REBASE-m");
        goto details;
    }
  
    // REBASE-APPLY
    snprintf(filepath, sizeof(filepath), "%s/rebase-apply", vcs_specific_dir);
    if (file_exists(filepath)) {
        read_first_line("rebase-apply/next", step, sizeof(step));
        read_first_line("rebase-apply/last", total, sizeof(total));
        
        snprintf(filepath, sizeof(filepath), "%s/rebase-apply/rebasing", vcs_specific_dir);
        if (file_exists(filepath)) {
            *vcs_head_status = strdup("REBASE");
        } else {
            *vcs_head_status = strdup("AM/REBASE");
        }
        goto details;
    }

    // CHERRY_PICK_HEAD
    snprintf(filepath, sizeof(filepath), "%s/CHERRY_PICK_HEAD", vcs_specific_dir);
    if (file_exists(filepath)) {
        *vcs_head_status = strdup("CHERRY-PICKING");
        return 0;
    }
  
    // REVERT_HEAD
    snprintf(filepath, sizeof(filepath), "%s/REVERT_HEAD", vcs_specific_dir);
    if (file_exists(filepath)) {
        *vcs_head_status = strdup("REVERTING");
        return 0;
    }
  
    // BISECT_START
    snprintf(filepath, sizeof(filepath), "%s/BISECT_START", vcs_specific_dir);
    if (file_exists(filepath)) {
        *vcs_head_status = strdup("BISECTING");
        return 0;
    }

    // Nothing found
    return 1;

details:
    if (strlen(step) > 0 && strlen(total) > 0) {
        snprintf(filepath, sizeof(filepath), "%s/%s", step, total);
        *vcs_head_details = strdup(filepath);
    }
    return 0;
}

int main() {
    char *status = NULL;
    char *details = NULL;

    if (git_head_status(".git", &status, &details) == 0) {
        printf("Status: %s\n", status);
        if (details) {
            printf("Details: %s\n", details);
        }
    }

    // Free allocated strings
    free(status);
    free(details);

    return 0;
}

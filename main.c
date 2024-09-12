// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_PATH_MAX 512
#define TIMESTAMP_MAX 80

const char target_dir[] = "/Users/shaneshort/Documents/Notes/obs-cli/obs-cli/";
char current_dir[1024];

void create_note();
void edit_note(const char *filepath);
void list_notes();

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [arguments]\n", argv[0]);
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  add                  Create a new note\n");
        fprintf(stderr, "  edit <filepath>      Edit an existing note\n");
        fprintf(stderr, "  list                 List all notes\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "add") == 0) {
        create_note();
    } else if (strcmp(argv[1], "edit") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'edit' command requires a filepath argument.\n");
            return EXIT_FAILURE;
        }
        edit_note(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        list_notes();
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Function to create a new note
void create_note() {
    char file_path[FILE_PATH_MAX];
    char timestamp[TIMESTAMP_MAX];

    generate_timestamp(timestamp, sizeof(timestamp));

    if (is_git_repository()) {
        char *url = get_remote_url();
        if (url) {
            char git_organisation[256] = {0};
            char repo_name[256] = {0};
            parse_url(url, git_organisation, repo_name);

            char user_dir[FILE_PATH_MAX];
            char repo_dir[FILE_PATH_MAX];
            
            // Create the path up to the git organization folder
            snprintf(user_dir, sizeof(user_dir), "%s/%s", target_dir, git_organisation);

            // Ensure the git organization directory exists
            if (!dir_exists(user_dir)) {
                create_directory(user_dir);
            }

            // Create the path for the repository directory under the git organization
            snprintf(repo_dir, sizeof(repo_dir), "%s/%s", user_dir, repo_name);

            // Ensure the repository directory exists
            if (!dir_exists(repo_dir)) {
                create_directory(repo_dir);
            }

            // Create the final file path including the timestamp as a subdirectory
            snprintf(file_path, sizeof(file_path), "%s/%s/%s.md", user_dir, repo_name, timestamp);

            // Create the note file
            FILE *fp = fopen(file_path, "w");
            if (fp) {
                printf("File '%s' created.\n", file_path);
                fclose(fp);
            } else {
                perror("Failed to create file");
            }
        } else {
            printf("Failed to retrieve remote URL.\n");
        }
    } else {
        // Not a Git repository, create note in a temporary location
        char temp_dir[FILE_PATH_MAX];
        snprintf(temp_dir, sizeof(temp_dir), "%s/temp", target_dir);

        // Ensure the temp directory exists
        if (!dir_exists(temp_dir)) {
            create_directory(temp_dir);
        }

        // Create the file path using the timestamp as the filename in the temp directory
        snprintf(file_path, sizeof(file_path), "%s/temp/%s.md", target_dir, timestamp);

        // Create the note file
        FILE *file = fopen(file_path, "w");
        if (file) {
            fprintf(file, "New note created.\n");
            fclose(file);
            printf("File '%s' created.\n", file_path);
        } else {
            perror("Error opening file");
        }
    }

    // Open the created file in Neovim
    char vim_command[FILE_PATH_MAX + 6];  // Extra space for "nvim " and null terminator
    snprintf(vim_command, sizeof(vim_command), "nvim %s", file_path);
    int status = system(vim_command);
    if (status == -1) {
        perror("Error executing Neovim");
    }
}


void edit_note(const char *filepath) {
    // Set the current directory for autocomplete
    set_current_dir(target_dir);
    
    // Prompt for file path with auto-completion
    char *input;
    while ((input = readline("Enter file path: ")) != NULL) {
        if (strlen(input) > 0) {
            add_history(input);

            char full_path[FILE_PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, input);

            struct stat path_stat;
            if (stat(full_path, &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) {
                    change_directory(full_path);
                    printf("Changed directory to: %s\n", current_dir);
                } else if (S_ISREG(path_stat.st_mode)) {
                    printf("You are opening the file: %s\n", full_path);
                    char vim_command[FILE_PATH_MAX + 6];
                    snprintf(vim_command, sizeof(vim_command), "nvim %s", full_path);
                    int status = system(vim_command);
                    if (status == -1) {
                        perror("Error executing Neovim");
                    }
                    break; // Exit the loop after opening the file
                }
            } else {
                printf("Invalid path: %s\n", input);
            }
        }
        free(input);
    }
}
// Function to list all notes
void list_notes() {
    char list_command[FILE_PATH_MAX];
    snprintf(list_command, sizeof(list_command), "find %s -name '*.md'", target_dir);
    int status = system(list_command);
    if (status == -1) {
        perror("Error listing notes");
    }
}


// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../utils/utils.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FILE_PATH_MAX 512
#define TIMESTAMP_MAX 80
#define OBS_CONFIG_FILE ".obs"

char target_dir[128];
char current_dir[1024];

void create_note();
void edit_note(const char *filepath);
void list_notes();
void config_target_dir();
int load_target_dir_from_config();
void write_target_dir_to_config(const char *path);

int main(int argc, char *argv[]) {
    // Check if 'config' command is issued before attempting to load the target directory
    if (argc >= 2 && strcmp(argv[1], "config") == 0) {
        config_target_dir(); // Handle config command immediately
        return EXIT_SUCCESS;
    }

    // Attempt to load the target directory from the config file
    if (!load_target_dir_from_config()) {
        fprintf(stderr, "Target directory not configured.\n");
        fprintf(stderr, "Run '%s config' to set the target directory.\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Proceed with other commands
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [arguments]\n", argv[0]);
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  add                  Create a new note\n");
        fprintf(stderr, "  edit <filepath>      Edit an existing note\n");
        fprintf(stderr, "  list                 List all notes\n");
        fprintf(stderr, "  config               Set or update the target directory\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "add") == 0) {
        create_note();
    } else if (strcmp(argv[1], "edit") == 0) {
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
    // Set the current directory for autocomplete to the target directory
    set_current_dir(target_dir);
    printf("current dir: %s\n", current_dir);

    // Configure the Readline auto-completion function to use our generator
    rl_attempted_completion_function = complete;

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

// Function to configure the target directory
void config_target_dir() {
    char *input = readline("Enter the target directory path: ");
    if (input && strlen(input) > 0) {
        write_target_dir_to_config(input);
        printf("Target directory set to: %s\n", input);
        free(input);
    } else {
        fprintf(stderr, "Invalid directory path.\n");
    }
}

// Function to load the target directory from the ~/.obs config file
int load_target_dir_from_config() {
    char config_path[FILE_PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/%s", getenv("HOME"), OBS_CONFIG_FILE);

    FILE *file = fopen(config_path, "r");
    if (!file) {
        return 0; // Config file doesn't exist
    }

    if (fgets(target_dir, sizeof(target_dir), file) == NULL) {
        fclose(file);
        return 0; // Failed to read target directory
    }

    // Remove trailing newline if present
    size_t len = strlen(target_dir);
    if (len > 0 && target_dir[len - 1] == '\n') {
        target_dir[len - 1] = '\0';
    }

    fclose(file);
    return 1; // Successfully loaded target directory
}

// Function to write the target directory to the ~/.obs config file
void write_target_dir_to_config(const char *path) {
    char config_path[FILE_PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/%s", getenv("HOME"), OBS_CONFIG_FILE);

    FILE *file = fopen(config_path, "w");
    if (!file) {
        perror("Failed to open config file");
        return;
    }

    fprintf(file, "%s\n", path);
    fclose(file);

    // Update the global target_dir variable
    strncpy(target_dir, path, sizeof(target_dir) - 1);
    target_dir[sizeof(target_dir) - 1] = '\0'; // Ensure null termination
}


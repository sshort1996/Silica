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
#define MAX_LINE_LENGTH 256

char target_dir[128];
char api_key[128];
char current_dir[1024];
char original_dir[FILE_PATH_MAX];

void create_note();
void edit_note(const char *filepath);
void list_notes();
void config_target_dir();
int load_target_dir_from_config();
void write_target_dir_to_config(const char *path, const char *key); 
char *send_prompt(const char *root_directory, const char *prompt, long prompt_size);     

int main(int argc, char *argv[]) {

    // Save the current working directory
    if (getcwd(original_dir, sizeof(original_dir)) == NULL) {
        perror("getcwd");
        return EXIT_FAILURE;
    }

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

char *send_prompt(const char *root_directory, const char *prompt, long prompt_size) {
    FILE *fp;
    char *new_filename = malloc(2048);  // Allocate memory for the new filename

    // Calculate the required size for the command string
    size_t command_size = strlen(root_directory) + prompt_size + 50; // extra space for "python3 " and other parts of the command
    char *command = malloc(command_size);

    if (command == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the Python command with the root directory and script path
    snprintf(command, command_size, "python3 %s/src/file_parsing.py \"%s\"", root_directory, prompt);

    // Open the command for reading
    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        free(command);
        free(new_filename);
        exit(1);
    }

    // Read the output from the Python script (expected to be the new filename)
    if (fgets(new_filename, 2048, fp) != NULL) {
        // Remove the trailing newline character if any
        new_filename[strcspn(new_filename, "\n")] = '\0';
    } else {
        perror("Error reading from Python script");
        free(new_filename);
        new_filename = NULL;
    }

    // Close the process and free the command string
    pclose(fp);
    free(command);

    return new_filename;  // Return the new filename
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
                return; // Early exit on error
            }
        } else {
            printf("Failed to retrieve remote URL.\n");
            return; // Early exit on error
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
            return; // Early exit on error
        }
    }

    // Open the created file in Neovim
    char vim_command[FILE_PATH_MAX + 6];  // Extra space for "nvim " and null terminator
    snprintf(vim_command, sizeof(vim_command), "nvim %s", file_path);
    int status = system(vim_command);
    if (status == -1) {
        perror("Error executing Neovim");
    }

    // After Neovim closes, read the file contents for renaming
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error opening file for reading");
        return; // Early exit on error
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_contents = malloc(file_size + 1);
    if (file_contents) {
        fread(file_contents, 1, file_size, file);
        file_contents[file_size] = '\0'; // Null-terminate the string
        fclose(file);

        // Pass the file contents to the processing function
        char *new_filename = send_prompt(original_dir, file_contents, file_size);
        printf("Suggested filename: %s\n", new_filename);

        // Get the directory part of the file_path
        char *last_slash = strrchr(file_path, '/'); // Find the last slash in the path
        if (last_slash != NULL) {
            // Extract the directory path
            size_t dir_length = last_slash - file_path + 1; // +1 to include the slash
            char file_dir[FILE_PATH_MAX];
            strncpy(file_dir, file_path, dir_length);
            file_dir[dir_length] = '\0'; // Null-terminate the directory string

            // Rename the file if a new filename was returned
            if (new_filename && strlen(new_filename) > 0) {
                char new_file_path[FILE_PATH_MAX];
                snprintf(new_file_path, sizeof(new_file_path), "%s%s.md", file_dir, new_filename); // Use file_dir instead of current_dir
                if (rename(file_path, new_file_path) == 0) {
                    printf("File renamed to: %s\n", new_file_path);
                } else {
                    perror("Error renaming file");
                }
            } else {
                printf("No new filename returned.\n");
            }
        }

        free(file_contents);
        free(new_filename);
    } else {
        perror("Memory allocation failed");
    }
}


void edit_note(const char *filepath) {
    // Set the current directory for autocomplete to the target directory
    set_current_dir(target_dir);
    printf("Current directory: %s\n", current_dir);

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
                    
                    // Open the file in Neovim
                    char vim_command[FILE_PATH_MAX + 6];  // Extra space for "nvim " and null terminator
                    snprintf(vim_command, sizeof(vim_command), "nvim %s", full_path);
                    if (system(vim_command) == -1) {
                        perror("Error executing Neovim");
                    }

                    // Read the file contents after closing Neovim
                    FILE *file = fopen(full_path, "r");
                    if (file == NULL) {
                        perror("Error opening file for reading");
                        free(input);
                        continue; 
                    }

                    fseek(file, 0, SEEK_END);
                    long file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    char *file_contents = malloc(file_size + 1);
                    if (file_contents) {
                        fread(file_contents, 1, file_size, file);
                        file_contents[file_size] = '\0'; 
                        fclose(file);

                        // Send the file contents to src/file-parsing.py to create a relevant filename
                        char *new_filename = send_prompt(original_dir, file_contents, file_size);
                        printf("Suggested filename: %s\n", new_filename);

                        // Get the directory part of the full_path
                        char *last_slash = strrchr(full_path, '/');
                        if (last_slash != NULL) {
                            // Extract the directory path
                            size_t dir_length = last_slash - full_path + 1;
                            char file_dir[FILE_PATH_MAX];
                            strncpy(file_dir, full_path, dir_length);
                            file_dir[dir_length] = '\0';

                            // Rename the file if a new filename was returned
                            if (new_filename && strlen(new_filename) > 0) {
                                char new_file_path[FILE_PATH_MAX];
                                snprintf(new_file_path, sizeof(new_file_path), "%s%s.md", file_dir, new_filename);
                                if (rename(full_path, new_file_path) == 0) {
                                    printf("File renamed to: %s\n", new_file_path);
                                } else {
                                    perror("Error renaming file");
                                }
                            } else {
                                printf("No new filename returned.\n");
                            }
                        }

                        free(file_contents);
                        free(new_filename);
                    } else {
                        perror("Memory allocation failed");
                        fclose(file);
                    }
                    break;
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

void config_target_dir() {
    // Prompt for the target directory
    char *target_input = readline("Enter the target directory path: ");
    if (!target_input || strlen(target_input) == 0) {
        fprintf(stderr, "Invalid directory path.\n");
        free(target_input);
        return;
    }

    // Prompt for the API key
    char *api_input = readline("Enter the OpenAI API key: ");
    if (!api_input || strlen(api_input) == 0) {
        fprintf(stderr, "Invalid API key.\n");
        free(target_input);
        free(api_input);
        return;
    }

    // Write both values to the config file
    write_target_dir_to_config(target_input, api_input);

    printf("Target directory set to: %s\n", target_input);
    printf("API key set.\n");

    // Free the allocated memory for inputs
    free(target_input);
    free(api_input);
}

int load_target_dir_from_config() {
    char config_path[FILE_PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/%s", getenv("HOME"), OBS_CONFIG_FILE);

    FILE *file = fopen(config_path, "r");
    if (!file) {
        return 0; // Config file doesn't exist
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Check for the target_dir line
        if (strncmp(line, "TARGET_DIR=", 11) == 0) {
            strncpy(target_dir, line + 11, sizeof(target_dir) - 1);
            target_dir[sizeof(target_dir) - 1] = '\0';  // Ensure null-termination
        }

        // Check for the api_key line
        else if (strncmp(line, "OPEN_AI_API_KEY=", 16) == 0) {
            strncpy(api_key, line + 16, sizeof(api_key) - 1);
            api_key[sizeof(api_key) - 1] = '\0';  // Ensure null-termination
        }
    }

    fclose(file);

    // Verify both target_dir and api_key were loaded
    if (strlen(target_dir) > 0 && strlen(api_key) > 0) {
        return 1; // Successfully loaded target_dir and api_key
    }

    return 0; // Failed to load both values
}

void write_target_dir_to_config(const char *path, const char *key) {
    char config_path[FILE_PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/%s", getenv("HOME"), OBS_CONFIG_FILE);

    FILE *file = fopen(config_path, "w");
    if (!file) {
        perror("Failed to open config file");
        return;
    }

    // Write the target directory and the API key to the config file
    fprintf(file, "TARGET_DIR=%s\n", path);
    fprintf(file, "OPEN_AI_API_KEY=%s\n", key);
    fclose(file);

    // Update the global target_dir and api_key variables
    strncpy(target_dir, path, sizeof(target_dir) - 1);
    target_dir[sizeof(target_dir) - 1] = '\0'; // Ensure null termination

    strncpy(api_key, key, sizeof(api_key) - 1);
    api_key[sizeof(api_key) - 1] = '\0'; // Ensure null termination
}


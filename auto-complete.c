#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define INITIAL_DIR "/Users/shaneshort/Documents/Notes/obs-cli/obs-cli/"

// Keeps track of the current working directory
char current_dir[1024];

// Function to set the current working directory
void set_current_dir(const char *path) {
    strncpy(current_dir, path, sizeof(current_dir) - 1);
    current_dir[sizeof(current_dir) - 1] = '\0';  // Ensure null termination
}

// Function to change the directory
void change_directory(const char *path) {
    if (chdir(path) == 0) {
        set_current_dir(path);
    } else {
        perror("chdir");
    }
}

// Generator function to return matches one by one
char *generator(const char *text, int state) {
    static DIR *dir;
    static struct dirent *entry;
    static int len;
    static char directory[1024];
    static char *last_slash;

    // Determine the base directory to open based on the input
    if (state == 0) {  // Reset state when a new text input is processed
        if ((last_slash = strrchr(text, '/')) != NULL) {
            // If there is a slash, open the specified subdirectory
            snprintf(directory, sizeof(directory), "%s/%.*s", current_dir, (int)(last_slash - text), text);
        } else {
            // Otherwise, open the current directory
            strncpy(directory, current_dir, sizeof(directory));
        }

        // Open the directory for the first time
        if (dir) closedir(dir);  // Close previously opened directory
        dir = opendir(directory);
        if (dir == NULL) {
            perror("opendir");
            return NULL;
        }

        len = last_slash ? strlen(last_slash + 1) : strlen(text);  // Length of text after the last slash
    }

    // Read directory entries and match against the input text
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')  // Skip hidden files and directories
            continue;

        // Compare based on the length after the last slash
        if (strncmp(entry->d_name, last_slash ? last_slash + 1 : text, len) == 0) {
            // Allocate space for the result and build it with full path
            char *result;
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

            struct stat path_stat;
            if (stat(full_path, &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) {  // Directory found
                    result = (char *)malloc(strlen(text) + strlen(entry->d_name) + 2);  // +1 for '/' and +1 for '\0'
                    snprintf(result, strlen(text) - len + 1, "%s", text);  // Copy the path up to the last slash
                    strcat(result, entry->d_name);
                    strcat(result, "/");
                } else {  // File found
                    result = (char *)malloc(strlen(text) + strlen(entry->d_name) + 1);
                    snprintf(result, strlen(text) - len + 1, "%s", text);  // Copy the path up to the last slash
                    strcat(result, entry->d_name);
                }
                return result;
            }
        }
    }

    closedir(dir);  // Close directory when done
    dir = NULL;     // Reset directory pointer
    return NULL;    // No more matches found
}

// The completion function called by readline to generate matches
char **complete(const char *text, int start, int end) {
    rl_completion_append_character = '\0';  // Suppress any appended character

    char **matches = rl_completion_matches(text, generator);
    return matches;
}

int main(int argc, char *argv[]) {
    // Set the initial directory
    set_current_dir(INITIAL_DIR);
    printf("Tab to view suggestions");
    // Set up the readline library
    rl_attempted_completion_function = complete;  // Set the completion function

    // Readline loop
    char *input;
    while ((input = readline("")) != NULL) {
        if (strlen(input) > 0) {
            add_history(input);

            // Build the full path
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, input);

            struct stat path_stat;
            if (stat(full_path, &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) {  // If it's a directory, change to it
                    change_directory(full_path);
                    printf("Changed directory to: %s\n", current_dir);
                } else if (S_ISREG(path_stat.st_mode)) {  // If it's a file, print the path
                    printf("You are opening the file: %s\n", full_path);
                    // Optionally, use system("vim full_path"); to open the file in vim
                }
            } else {
                printf("Invalid path: %s\n", input);
            }
        }
        free(input);
    }

    return 0;
}


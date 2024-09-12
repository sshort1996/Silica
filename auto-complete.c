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

    if (state == 0) {
        // Open the directory for the first time
        if (dir) closedir(dir);  // Close previously opened directory
        dir = opendir(current_dir);
        if (dir == NULL) {
            perror("opendir");
            return NULL;
        }
        len = strlen(text);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')  // Skip hidden files and directories
            continue;

        if (strncmp(entry->d_name, text, len) == 0) {
            // Allocate space for the path and check if it is a directory
            char *result;
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

            struct stat path_stat;
            if (stat(full_path, &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) {  // Directory found
                    result = (char *)malloc(strlen(entry->d_name) + 2);  // +1 for '/' and +1 for '\0'
                    strcpy(result, entry->d_name);
                    strcat(result, "/");
                } else {  // File found
                    result = strdup(entry->d_name);
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
    // Generate the matches using rl_completion_matches
    return rl_completion_matches(text, generator);
}

int main(int argc, char *argv[]) {
    // Set the initial directory
    set_current_dir(INITIAL_DIR);

    // Set up the readline library
    rl_attempted_completion_function = complete;  // Set the completion function

    // Readline loop
    char *input;
    while ((input = readline("Enter file path: ")) != NULL) {
        if (strlen(input) > 0) {
            add_history(input);

            // Check if the input is a directory and change to it
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, input);

            struct stat path_stat;
            if (stat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
                change_directory(full_path);
                printf("Changed directory to: %s\n", current_dir);
            } else {
                printf("You entered: %s\n", input);
            }
        }
        free(input);
    }

    return 0;
}


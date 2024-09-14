// utils.c
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <unistd.h>

#define CWD_PATH_SIZE 128

// Function to check if we're in a git repository
int is_git_repository() {
    FILE *fp;
    char path[CWD_PATH_SIZE];
    int is_git = 0;

    fp = popen("git rev-parse --is-inside-work-tree 2>&1", "r");
    if (fp == NULL) {
        perror("popen failed");
        return -1;
    }

    if (fgets(path, sizeof(path) - 1, fp) != NULL) {
        if (strstr(path, "true") != NULL) {
            is_git = 1;
        }
    }

    pclose(fp);
    return is_git;
}

char* get_remote_url() {
    FILE *fp;
    static char remote_url[CWD_PATH_SIZE];

    fp = popen("git remote get-url origin 2>&1", "r");
    if (fp == NULL) {
        perror("popen failed");
        return NULL;
    }

    if (fgets(remote_url, sizeof(remote_url) - 1, fp) != NULL) {
        size_t len = strlen(remote_url);
        if (len > 0 && remote_url[len - 1] == '\n') {
            remote_url[len - 1] = '\0';
        }
    } else {
        pclose(fp);
        return NULL;
    }

    pclose(fp);
    return remote_url;
}

// Function to generate timestamp
void generate_timestamp(char *timestamp, size_t size) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (strftime(timestamp, size, "%Y-%m-%d_%H-%M-%S", timeinfo) == 0) {
        fprintf(stderr, "Error formatting timestamp.\n");
        exit(EXIT_FAILURE);
    }
}

// Function to extract the git_organisation and repository name from the URL
void parse_url(const char* url, char* git_organisation, char* repo_name) {
    char *at_ptr, *colon_ptr, *slash_ptr;

    at_ptr = strchr(url, '@'); // For SSH URLs
    colon_ptr = strchr(url, ':'); // For SSH URLs
    slash_ptr = strstr(url, "/"); // For HTTPS URLs

    if (at_ptr && colon_ptr) { // SSH format: git@github.com:git_organisation/repo.git
        sscanf(colon_ptr + 1, "%[^/]/%[^.]", git_organisation, repo_name);
    } else if (slash_ptr) { // HTTPS format: https://github.com/git_organisation/repo.git
        sscanf(slash_ptr + 1, "%[^/]/%[^.]", git_organisation, repo_name);
    }
}

// Function to create a directory
void create_directory(const char *path) {
    if (mkdir(path, 0777) != 0) {
        perror("Failed to create directory");
    } else {
        printf("Directory '%s' created.\n", path);
    }
}

// Function to check if a directory exists
int dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Function to check if a file exists
int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

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


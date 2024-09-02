#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h> // for chdir
#include <sys/stat.h> // for mkdir
#include <errno.h>

#define FILE_PATH_MAX 256
#define TIMESTAMP_MAX 80

const char target_dir[] = "test-dir"; // Global target directory (e.g., Obsidian vault location)
#define CWD_PATH_SIZE 128 // cwd path 

// function to check if we're in a git repository 
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
    char path[CWD_PATH_SIZE];
    static char remote_url[CWD_PATH_SIZE];

    fp = popen("git remote get-url origin 2>&1", "r");
    if (fp == NULL) {
        perror("popen failed");
        return NULL;
    }

    if (fgets(remote_url, sizeof(remote_url) - 1, fp) != NULL) {
        // Remove newline character if present
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

// Function to extract the username and repository name from the URL
void parse_url(const char* url, char* username, char* repo_name) {
    char *at_ptr, *colon_ptr, *slash_ptr;

    at_ptr = strchr(url, '@'); // For SSH URLs
    colon_ptr = strchr(url, ':'); // For SSH URLs
    slash_ptr = strstr(url, "/"); // For HTTPS URLs

    if (at_ptr && colon_ptr) { // SSH format: git@github.com:username/repo.git
        sscanf(colon_ptr + 1, "%[^/]/%[^.]", username, repo_name);
    } else if (slash_ptr) { // HTTPS format: https://github.com/username/repo.git
        sscanf(slash_ptr + 1, "%[^/]/%[^.]", username, repo_name);
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

int main(int argc, char *argv[]) {
    char file_path[FILE_PATH_MAX];
    char timestamp[TIMESTAMP_MAX];

    // Generate the timestamp
    generate_timestamp(timestamp, sizeof(timestamp));

    // Print the formatted date and time
    printf("Current date and time: %s\n", timestamp);

    // check if we're in a git repository
    if (is_git_repository()) {
        char *url = get_remote_url();
        if (url) {
            char username[256] = {0};
            char repo_name[256] = {0};

            // Parse the URL to get the username and repository name
            parse_url(url, username, repo_name);
            printf("Username: %s, Repository Name: %s\n", username, repo_name);

            // Check if directory exists
            if (!dir_exists(username)) {
                // Create directory
                if (mkdir(username, 0777) != 0) {
                    perror("Failed to create directory");
                } else {
                    printf("Directory '%s' created.\n", username);
                }
            } else {
                printf("Directory '%s' already exists.\n", username);
            }

            // Check if file exists
            snprintf(file_path, sizeof(file_path), "%s/temp/%s.txt", username, repo_name);
            if (!file_exists(file_path)) {
                // Create file
                FILE *fp = fopen(file_path, "w");
                if (fp == NULL) {
                    perror("Failed to create file");
                } else {
                    printf("File '%s' created.\n", file_path);
                    fclose(fp);
                }
            } else {
                printf("File '%s' already exists.\n", file_path);
            }

        } else {
            printf("Failed to retrieve remote URL.\n");
        }
    } else {
        printf("Not a Git repository. Creating note in a temp location\n");
        if (snprintf(file_path, sizeof(file_path), "%s/temp/%s.txt", target_dir, timestamp) >= sizeof(file_path)) {
            fprintf(stderr, "File path is too long.\n");
            return EXIT_FAILURE;
        }
    }


    // Open the file for writing
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    if (argc > 1) {
        // Write the command-line argument to the file
        fprintf(file, "Hello, World!\n");
        fputs(argv[1], file);
    } else {
        // If no command-line arguments, open a Vim editor to write the file
        printf("Opening Vim...\n");
        char vim_command[FILE_PATH_MAX + 5]; // Extra space for "vim " and null terminator

        if (snprintf(vim_command, sizeof(vim_command), "vim %s", file_path) >= sizeof(vim_command)) {
            fprintf(stderr, "Vim command is too long.\n");
            fclose(file);
            return EXIT_FAILURE;
        }

        int status = system(vim_command);
        if (status == -1) {
            perror("Error executing Vim");
            fclose(file);
            return EXIT_FAILURE;
        }
    }

    fclose(file);
    printf("Note written successfully.\n");

    return EXIT_SUCCESS;
}


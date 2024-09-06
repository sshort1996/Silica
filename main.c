#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h> // for chdir
#include <sys/stat.h> // for mkdir
#include <errno.h>

#define FILE_PATH_MAX 512  // Increased to account for longer paths
#define TIMESTAMP_MAX 80

const char target_dir[] = "/Users/shaneshort/Documents/Notes/obs-cli/obs-cli/";
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

int main(int argc, char *argv[]) {
    char file_path[FILE_PATH_MAX];
    char timestamp[TIMESTAMP_MAX];

    // Generate the timestamp
    generate_timestamp(timestamp, sizeof(timestamp));

    // Print the formatted date and time
    printf("Current date and time: %s\n", timestamp);

    // Check if we're in a git repository
    if (is_git_repository()) {
        char *url = get_remote_url();
        if (url) {
            char username[256] = {0};
            char repo_name[256] = {0};

            // Parse the URL to get the username and repository name
            parse_url(url, username, repo_name);
            printf("Username: %s, Repository Name: %s\n", username, repo_name);

            // Create the full path for the username directory under target_dir
            char user_dir[FILE_PATH_MAX];
            snprintf(user_dir, sizeof(user_dir), "%s/%s", target_dir, username);

            // Check if directory exists
            if (!dir_exists(user_dir)) {
                create_directory(user_dir);
            } else {
                printf("Directory '%s' already exists.\n", user_dir);
            }

            // Prepare the file path in the username directory
            snprintf(file_path, sizeof(file_path), "%s/%s.md", user_dir, repo_name);

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

        // Use target_dir and timestamp to create the note file
        if (snprintf(file_path, sizeof(file_path), "%s/temp/%s.md", target_dir, timestamp) >= sizeof(file_path)) {
            fprintf(stderr, "File path is too long.\n");
            return EXIT_FAILURE;
        }

        // Ensure the temp directory exists
        char temp_dir[FILE_PATH_MAX];
        snprintf(temp_dir, sizeof(temp_dir), "%s/temp", target_dir);

        if (!dir_exists(temp_dir)) {
            create_directory(temp_dir);
        }

        // Create the note file in the temp directory
        if (!file_exists(file_path)) {
            FILE *file = fopen(file_path, "w");
            if (file == NULL) {
                perror("Error opening file");
                return EXIT_FAILURE;
            }

            if (argc > 1) {
                // Write the command-line argument to the file
                fprintf(file, "Hello, World!\n");
                fputs(argv[1], file);
            }
            fclose(file);
        } else {
            printf("File '%s' already exists.\n", file_path);
        }
    }

    // Open the file in Vim
    printf("Opening Neovim...\n");
    char vim_command[FILE_PATH_MAX + 6]; // Extra space for "vim " and null terminator

    if (snprintf(vim_command, sizeof(vim_command), "nvim %s", file_path) >= sizeof(vim_command)) {
        fprintf(stderr, "Vim command is too long.\n");
        return EXIT_FAILURE;
    }

    int status = system(vim_command);
    if (status == -1) {
        perror("Error executing Neovim");
        return EXIT_FAILURE;
    }

    printf("Note written successfully.\n");

    return EXIT_SUCCESS;
}


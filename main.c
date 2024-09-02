#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

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
            printf("Remote URL: %s\n", url);
            
        } else {
            printf("Failed to retrieve remote URL.\n");
        }
    } else {
        printf("Not a Git repository.\n");
    }

    // Create the file path
    if (snprintf(file_path, sizeof(file_path), "%s/%s.txt", target_dir, timestamp) >= sizeof(file_path)) {
        fprintf(stderr, "File path is too long.\n");
        return EXIT_FAILURE;
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


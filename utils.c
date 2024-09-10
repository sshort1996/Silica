// utils.c
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

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


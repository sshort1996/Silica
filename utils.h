// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <readline/readline.h>
#include <readline/history.h>

// Function declarations
int is_git_repository();
char* get_remote_url();
void generate_timestamp(char *timestamp, size_t size);
void parse_url(const char* url, char* git_organisation, char* repo_name);
void create_directory(const char *path);
int dir_exists(const char *path);
int file_exists(const char *path);
char **complete(const char *text, int start, int end);
char *generator(const char *text, int state);

#endif // UTILS_H


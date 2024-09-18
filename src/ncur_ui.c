#include <ncurses.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_ITEMS 1024
#define MAX_PATH_LEN 1024

// Struct to store file/directory information
typedef struct {
    char name[MAX_PATH_LEN];
    int is_dir;
} FileItem;

// Global variables to store current working directory and items
FileItem items[MAX_ITEMS];
int total_items = 0;
int current_selection = 0;
char current_dir[MAX_PATH_LEN];

// Function to load the files in the current directory
void load_directory(const char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat st;

    if (!dir) {
        perror("opendir");
        return;
    }

    total_items = 0;
    strcpy(current_dir, path);

    while ((entry = readdir(dir)) != NULL && total_items < MAX_ITEMS) {
        snprintf(items[total_items].name, MAX_PATH_LEN, "%s", entry->d_name);
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, MAX_PATH_LEN, "%s/%s", path, entry->d_name);
        stat(full_path, &st);
        items[total_items].is_dir = S_ISDIR(st.st_mode);
        total_items++;
    }

    closedir(dir);
    current_selection = 0;
}

// Function to display items on the screen
void display_items() {
    clear();
    for (int i = 0; i < total_items; i++) {
        if (i == current_selection) {
            attron(A_REVERSE);
        }
        if (items[i].is_dir) {
            printw("[DIR] %s\n", items[i].name);
        } else {
            printw("      %s\n", items[i].name);
        }
        if (i == current_selection) {
            attroff(A_REVERSE);
        }
    }
    refresh();
}

// Function to change directory
void change_directory(const char *new_dir) {
    char path[MAX_PATH_LEN];
    if (strcmp(new_dir, "..") == 0) {
        // Go up to parent directory
        char *last_slash = strrchr(current_dir, '/');
        if (last_slash) {
            *last_slash = '\0';
        }
    } else {
        snprintf(path, MAX_PATH_LEN, "%s/%s", current_dir, new_dir);
        strcpy(current_dir, path);
    }
    load_directory(current_dir);
}

int main() {
    // Initialize ncurses
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    // Get the current directory and load its content
    getcwd(current_dir, sizeof(current_dir));
    load_directory(current_dir);

    int ch;
    while (1) {
        display_items();
        ch = getch();

        switch (ch) {
            case 'k': // Move up
                if (current_selection > 0) {
                    current_selection--;
                }
                break;
            case 'j': // Move down
                if (current_selection < total_items - 1) {
                    current_selection++;
                }
                break;
            case 'h': // Go up one directory (parent)
                change_directory("..");
                break;
            case 'l': // Enter directory or open file
                if (items[current_selection].is_dir) {
                    change_directory(items[current_selection].name);
                } else {
                    // Open file in editor (e.g., using nvim)
                    char command[MAX_PATH_LEN + 10];
                    snprintf(command, sizeof(command), "nvim %s/%s", current_dir, items[current_selection].name);
                    endwin(); // Exit ncurses before opening the editor
                    system(command);
                    initscr(); // Reinitialize ncurses after editor closes
                    noecho();
                    curs_set(FALSE);
                    keypad(stdscr, TRUE);
                }
                break;
            case '\n': // Enter key (similar to 'l')
                if (items[current_selection].is_dir) {
                    change_directory(items[current_selection].name);
                } else {
                    // Open file in editor (e.g., using nvim)
                    char command[MAX_PATH_LEN + 10];
                    snprintf(command, sizeof(command), "nvim %s/%s", current_dir, items[current_selection].name);
                    endwin(); // Exit ncurses before opening the editor
                    system(command);
                    initscr(); // Reinitialize ncurses after editor closes
                    noecho();
                    curs_set(FALSE);
                    keypad(stdscr, TRUE);
                }
                break;
            case 'q': // Quit the application
                endwin();
                return 0;
            default:
                break;
        }
    }

    endwin();
    return 0;
}


#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <locale.h>

#define ASCII_ART_FILE "/Users/shaneshort/Documents/Development/noodling/obs-cli/static/ascii_logo.txt"
#define CONFIG_FILE_PATH "/Users/shaneshort/obs/.config"
#define MAX_LINES_PER_PAGE 13

// Define menu options
char *menu_items[] = {
    "Configuration",
    "View vault",
    "Create a new note",
    "Edit an existing note",
    "Clean up a rough note"
};

// Function to display ASCII art
void display_ascii_art(int start_row, int col) {
    FILE *file = fopen(ASCII_ART_FILE, "r");
    if (file == NULL) {
        mvprintw(start_row, col, "Failed to open ASCII art file.");
        return;
    }

    char line[256];
    int current_row = start_row;

    // Read each line from the file and print it to the screen
    while (fgets(line, sizeof(line), file)) {
        mvprintw(current_row++, col, "%s", line);
    }

    fclose(file);
}

void draw_rounded_box(int start_row, int start_col, int height, int width) {
    mvaddch(start_row, start_col, ACS_ULCORNER);
    mvaddch(start_row, start_col + width - 1, ACS_URCORNER);
    mvaddch(start_row + height - 1, start_col, ACS_LLCORNER);
    mvaddch(start_row + height - 1, start_col + width - 1, ACS_LRCORNER);

    for (int i = 1; i < width - 1; i++) {
        mvaddch(start_row, start_col + i, ACS_HLINE); // Top horizontal line
        mvaddch(start_row + height - 1, start_col + i, ACS_HLINE); // Bottom horizontal line
    }

    for (int i = 1; i < height - 1; i++) {
        mvaddch(start_row + i, start_col, ACS_VLINE); // Left vertical line
        mvaddch(start_row + i, start_col + width - 1, ACS_VLINE); // Right vertical line
    }
}

// Function to print multi-line text within a box
void print_multiline(int start_row, int start_col, const char *message, int width) {
    int line_start = 0;
    int message_length = strlen(message);

    // Calculate the usable width inside the box (excluding the border)
    int usable_width = width - 2;

    while (line_start < message_length) {
        // Calculate the length of the current line (clip if message exceeds the usable width)
        int line_length = usable_width;

        if (line_start + line_length > message_length) {
            line_length = message_length - line_start;
        }

        // Check for a newline or line break
        char *newline = strchr(message + line_start, '\n');
        if (newline && (newline - message) < line_start + line_length) {
            line_length = newline - (message + line_start);  // Adjust to the newline
        }

        // Copy the line to a temporary buffer for printing
        char line_buffer[usable_width + 1];
        strncpy(line_buffer, message + line_start, line_length);
        line_buffer[line_length] = '\0';

        // Print the line, ensuring it starts within the box boundaries
        mvprintw(start_row++, start_col + 1, "%s", line_buffer);

        // Skip past the processed part of the message
        line_start += line_length;

        // If the line ends with a newline, skip it
        if (newline && line_start == (newline - message)) {
            line_start++;
        }
    }
}

// Function to read the configuration file and return its contents
char* read_config_file(const char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return strdup("Failed to open configuration file.");
    }

    char *buffer = NULL;
    size_t total_size = 0;
    char temp_buffer[256];
    
    while (fgets(temp_buffer, sizeof(temp_buffer), file) != NULL) {
        size_t temp_len = strlen(temp_buffer);
        char *new_buffer = realloc(buffer, total_size + temp_len + 1);
        
        if (new_buffer == NULL) {
            free(buffer);
            fclose(file);
            return strdup("Memory allocation error.");
        }

        buffer = new_buffer;
        strcpy(buffer + total_size, temp_buffer);
        total_size += temp_len;
    }

    fclose(file);

    if (total_size == 0) {
        free(buffer);
        return strdup("Configuration file is empty.");
    }

    return buffer;
}

char** split_output_into_lines(char* output, int* total_lines) {
    char** lines = NULL;
    char* line = strtok(output, "\n");
    int count = 0;

    while (line != NULL) {
        lines = realloc(lines, sizeof(char*) * (count + 1));
        lines[count] = strdup(line);
        count++;
        line = strtok(NULL, "\n");
    }

    *total_lines = count;
    return lines;
}

char** run_sil_list(int* total_lines) {
    FILE *fp;
    char *buffer = NULL;
    size_t total_size = 0;

    fp = popen("silica list", "r");
    if (fp == NULL) {
        *total_lines = 0;
        return NULL;
    }

    char temp_buffer[256];
    while (fgets(temp_buffer, sizeof(temp_buffer), fp) != NULL) {
        size_t temp_len = strlen(temp_buffer);
        char *new_buffer = realloc(buffer, total_size + temp_len + 1);
        if (new_buffer == NULL) {
            free(buffer);
            pclose(fp);
            *total_lines = 0;
            return NULL;
        }
        buffer = new_buffer;
        strcpy(buffer + total_size, temp_buffer);
        total_size += temp_len;
    }

    pclose(fp);

    // Check if no data was read
    if (total_size == 0) {
        free(buffer);
        *total_lines = 0;
        return NULL;
    }

    return split_output_into_lines(buffer, total_lines);
}

char* run_sil_add() {
    FILE *fp;
    char *buffer = NULL;
    size_t total_size = 0;

    fp = popen("silica add", "r");
    if (fp == NULL) {
        return NULL;
    }

    char temp_buffer[256];
    while (fgets(temp_buffer, sizeof(temp_buffer), fp) != NULL) {
        size_t temp_len = strlen(temp_buffer);
        char *new_buffer = realloc(buffer, total_size + temp_len + 1);
        if (new_buffer == NULL) {
            free(buffer);
            pclose(fp);
            return NULL;
        }
        buffer = new_buffer;
        strcpy(buffer + total_size, temp_buffer);
        total_size += temp_len;
    }

    pclose(fp);

    if (total_size == 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

int main() {
    setlocale(LC_ALL, "");

    initscr();
    clear();
    noecho();
    cbreak();
    keypad(stdscr, TRUE); // Enable special keys to be captured

    // Initialize color support
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK); // Define color pair (foreground, background)

    // Get the screen size
    int row, col;
    getmaxyx(stdscr, row, col);

    // Menu setup
    int num_choices = sizeof(menu_items) / sizeof(menu_items[0]);
    int highlight = 0; // Keeps track of the currently highlighted item
    int choice = 0;    // Keeps track of the selected choice

    // Declare variables for vault management
    char **vault_lines = NULL; // Lines for the vault
    int total_lines = 0;       // Total number of lines
    int total_pages = 0;       // Total pages for vault lines
    int current_page = 0;      // Current page of vault lines

    // Calculate vertical centering for menu and ASCII art
    int menu_start_row = (row - num_choices) / 2 - 6;
    int ascii_start_row = row / 6 - 2;
    int selected_opt_row = row - 21;

    // Instructions message
    char *instructions = "Use hjkl to navigate, Enter to select, i to edit config, n/N to navigate pages, and q to quit";
    char *welcome_message = "Silica CLI: Here you can view, create, edit, and otherwise manage notes in your obsidian vault.";

    // Variable to hold configuration file contents
    char *config_contents = NULL;
    char *add_message = NULL;

    // Main loop
    while (1) {
        clear();

        // Draw boxes around elements
        draw_rounded_box(ascii_start_row, 8, 6, 44);// Box for ASCII art
        draw_rounded_box(menu_start_row, 8, num_choices + 2, 25); // Box for menu
        draw_rounded_box(row - 2, 0, 3, col); // Box for instructions
        draw_rounded_box(selected_opt_row, 8, 16, 55); // Box for selected option display

        // Print welcome message using the new function
        print_multiline(ascii_start_row + 1, 9, welcome_message, 43); // 44 is the box width
        display_ascii_art(ascii_start_row, 70);

        // Display the menu on the left side, centered vertically
        for (int i = 0; i < num_choices; ++i) {
            if (i == highlight) {
                attron(A_REVERSE);
            }
            mvprintw(menu_start_row + 1 + i, 10, menu_items[i]);
            attroff(A_REVERSE);
        }

        // Print instructions at the bottom
        mvprintw(row - 1, 2, instructions);

        // Print the selected option in the bottom right box
        if (highlight != 1 && highlight < num_choices) {
            mvprintw(selected_opt_row + 1, 9, "Selected: %s", menu_items[highlight]);
        }


        // Print the selected option in the bottom right box
        if (highlight == 1) {
            char buffer[100];
            snprintf(buffer, sizeof(buffer), "Selected: %s", menu_items[highlight]);

            // Calculate the position to print the selected item
            int left_column = 9;
            mvprintw(selected_opt_row + 1, left_column, "%s", buffer);

            // Calculate the position for the page number
            if (total_pages > 0) {
                char page_info[20];
                snprintf(page_info, sizeof(page_info), "[%d/%d]", current_page + 1, total_pages);

                int box_width = 55;
                int page_info_column = box_width - strlen(page_info) - 2; // Calculate column for right-justifying the page info

                // Print the page info at the calculated position
                mvprintw(selected_opt_row + 1, page_info_column, "%s", page_info);
            }
        }
        // Display configuration file contents if "Configuration" is selected
        if (highlight == 0 && config_contents) {
            attron(COLOR_PAIR(1));
            print_multiline(selected_opt_row + 2, 9, config_contents, 54);
            attroff(COLOR_PAIR(1));
        }

        // Display vault contents if "View vault" is selected
        if (highlight == 1) {
            if (vault_lines == NULL) {
                vault_lines = run_sil_list(&total_lines);
                total_pages = (total_lines + MAX_LINES_PER_PAGE - 1) / MAX_LINES_PER_PAGE;
                current_page = 0;
            }

            if (vault_lines != NULL) {
                attron(COLOR_PAIR(1));

                // Calculate the starting and ending line for current page
                int start_line = current_page * MAX_LINES_PER_PAGE;
                int end_line = (start_line + MAX_LINES_PER_PAGE < total_lines) ? start_line + MAX_LINES_PER_PAGE : total_lines;

                // Print the lines for the current page
                for (int i = start_line; i < end_line; i++) {
                    mvprintw(selected_opt_row + 2 + (i - start_line), 9, "%s", vault_lines[i]);
                }

                attroff(COLOR_PAIR(1));
            } else {
                mvprintw(selected_opt_row + 2, 9, "No output from obs list.");
            }
        }

        // Display stdout from `silica add` if "Create a new note" is selected
        if (highlight == 2) {
            attron(COLOR_PAIR(1));
            print_multiline(selected_opt_row + 2, 9, add_message, 54);
            attroff(COLOR_PAIR(1));
        }



        // Refresh the screen to show changes
        refresh();

        // Capture user input
        int c = getch();
        switch (c) {
            case 'k': // Move up 
                highlight = (highlight == 0) ? num_choices - 1 : highlight - 1;
                break;
            case 'j': // Move down
                highlight = (highlight == num_choices - 1) ? 0 : highlight + 1;
                break;
            case 10: // Enter key
                choice = highlight;
                if (choice == 0) { // Only if "Configuration" is selected
                    free(config_contents);
                    config_contents = read_config_file(CONFIG_FILE_PATH);
                }
                if (choice == 1) { // Only if "View vault" is selected
                    free(vault_lines);
                    vault_lines = NULL;
                }
                if (choice == 2) { // Only if "Create a new note" is selected
                    free(add_message);
                    add_message = run_sil_add();
                }
                break;
            case 'n': // Next page
                if (current_page < total_pages - 1) {
                    current_page++;
                }
                break;
            case 'N': // Previous page
                if (current_page > 0) {
                    current_page--;
                }
                break;
            case 'i': // Open configuration file in Neovim
                if (highlight == 0) {
                    // Close the ncurses window and run Neovim
                    endwin(); // End ncurses mode before running the command
                    system("nvim " CONFIG_FILE_PATH); // Open config file in Neovim
                    initscr(); // Reinitialize ncurses mode after returning
                    refresh();
                    // Reload the configuration file contents
                    free(config_contents); // Free previously allocated memory
                    config_contents = read_config_file(CONFIG_FILE_PATH); // Reload config contents
                }
                break;
            case 'q': // Exit on 'q' key
                free(config_contents);
                for (int i = 0; i < total_lines; i++) {
                    free(vault_lines[i]);
                }
                free(vault_lines);
                free(add_message);
                endwin();
                return 0;
        }
    }

    // Cleanup before exiting
    free(config_contents);
    for (int i = 0; i < total_lines; i++) {
        free(vault_lines[i]);
    }
    free(vault_lines);
    free(add_message);
    endwin();
    return 0;
}


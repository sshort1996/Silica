#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define ASCII_ART_FILE "/Users/shaneshort/Documents/Development/noodling/obs-cli/static/ascii_logo.txt"
#define CONFIG_FILE_PATH "/Users/shaneshort/obs/.config" // Replace USERNAME with your actual username

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
        mvprintw(current_row++, col, "%s", line);  // Print line at the specified position
    }

    fclose(file);
}

// Function to draw rounded corners box
void draw_rounded_box(int start_row, int start_col, int height, int width) {
    mvaddch(start_row, start_col, ACS_ULCORNER); // Upper left corner
    mvaddch(start_row, start_col + width - 1, ACS_URCORNER); // Upper right corner
    mvaddch(start_row + height - 1, start_col, ACS_LLCORNER); // Lower left corner
    mvaddch(start_row + height - 1, start_col + width - 1, ACS_LRCORNER); // Lower right corner

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
    int message_length = strlen(message);
    int line_start = 0;

    while (line_start < message_length) {
        // Calculate the length of the current line
        int line_length = width - 2; // Reserve space for borders
        if (line_start + line_length > message_length) {
            line_length = message_length - line_start; // Adjust for last line
        }

        // Check if the line is too long
        if (line_length == width - 2) {
            // If the line is too long, we need to find a space to split it
            int split_index = line_start + line_length;
            while (split_index > line_start && message[split_index] != ' ') {
                split_index--; // Move back to the last space
            }

            // If no space is found, we break at the max length
            if (split_index == line_start) {
                split_index = line_start + line_length; // Force break at max length
            }

            // Print the line
            char line[width]; // Create a temporary buffer
            strncpy(line, message + line_start, split_index - line_start);
            line[split_index - line_start] = '\0'; // Null-terminate the string
            mvprintw(start_row++, start_col, "%s", line); // Print the line
            line_start = split_index + 1; // Move to the next part of the message
        } else {
            // Print the last line
            char line[width];
            strncpy(line, message + line_start, line_length);
            line[line_length] = '\0'; // Null-terminate
            mvprintw(start_row++, start_col, "%s", line); // Print the line
            break; // Exit the loop
        }
    }
}

// Function to read the configuration file and return its contents
char* read_config_file(const char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return "Failed to open configuration file.";
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1); // +1 for null terminator
    if (buffer) {
        fread(buffer, 1, length, file);
        buffer[length] = '\0'; // Null-terminate the string

        // Replace new line characters with a space
        for (long i = 0; i < length; ++i) {
            if (buffer[i] == '\n') {
                buffer[i] = ' '; // Replace new line with space
            }
        }
    }

    fclose(file);
    return buffer;
}

// Function to run the 'obs list' command and return its output
char* run_obs_list() {
    FILE *fp;
    char *buffer = NULL;
    size_t size = 0;

    // Open the command for reading
    fp = popen("obs list", "r");
    if (fp == NULL) {
        return "Failed to run obs list command.";
    }

    // Read the output a line at a time - output it.
    while (getline(&buffer, &size, fp) != -1) {
        // Replace newline character with space for display
        buffer[strcspn(buffer, "\n")] = ' ';
    }

    // Close the command
    pclose(fp);
    return buffer;
}

int main() {
    // Initialize ncurses mode
    initscr();
    clear();
    noecho();
    cbreak(); // Disable line buffering, pass input directly
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

    // Calculate vertical centering for menu and ASCII art
    int menu_start_row = (row - num_choices) / 2 - 6; // Start the menu at the vertical center
    int ascii_start_row = row / 6 - 2; // Start ASCII art vertically above center
    int selected_opt_row = row - 21;

    // Instructions message
    char *instructions = "Use hjkl to navigate, Enter to select, i to edit config, q to quit";
    char *welcome_message = "Obsidian CLI: Here you can view, create, edit, and otherwise manage notes in your obsidian vault.";

    // Variable to hold configuration file contents
    char *config_contents = NULL;
    char *vault_contents = NULL; // Variable to hold vault contents

    // Main loop
    while (1) {
        // Clear the screen and print instructions and ASCII art
        clear();

        // Draw boxes around elements
        draw_rounded_box(ascii_start_row, 8, 6, 44); // Increased height for ASCII art box
        draw_rounded_box(menu_start_row, 8, num_choices + 2, 25); // Box for menu
        draw_rounded_box(row - 2, 0, 3, col); // Box for instructions
        draw_rounded_box(selected_opt_row, 8, 16, 44); // Box for selected option display

        // Print welcome message using the new function
        print_multiline(ascii_start_row + 1, 9, welcome_message, 44); // 44 is the box width
        display_ascii_art(ascii_start_row, (col - 20) / 2); // Adjust width as needed

        // Display the menu on the left side, centered vertically
        for (int i = 0; i < num_choices; ++i) {
            if (i == highlight) {
                attron(A_REVERSE); // Highlight the selected menu item
            }
            mvprintw(menu_start_row + 1 + i, 10, menu_items[i]); // Align menu items vertically centered
            attroff(A_REVERSE); // Turn off highlighting
        }

        // Print instructions at the bottom
        mvprintw(row - 1, 2, instructions);

        // Print the selected option in the bottom right box
        if (highlight >= 0 && highlight < num_choices) {
            mvprintw(selected_opt_row + 1, 9, "Selected: %s", menu_items[highlight]); // Print selection in box
        }

        // Display configuration file contents if "Configuration" is selected
        if (highlight == 0 && config_contents) {
            attron(COLOR_PAIR(1)); // Turn on the color pair for configuration content
            print_multiline(selected_opt_row + 2, 9, config_contents, 44); // Print contents in the box
            attroff(COLOR_PAIR(1)); // Turn off the color pair
        }

        // Display vault contents if "View vault" is selected
        if (highlight == 1 && vault_contents) {
            attron(COLOR_PAIR(1)); // Turn on the color pair for vault content
            print_multiline(selected_opt_row + 2, 9, vault_contents, 44); // Print contents in the box
            attroff(COLOR_PAIR(1)); // Turn off the color pair
        }

        // Refresh the screen to show changes
        refresh();

        // Capture user input
        int c = getch();
        switch (c) {
            case 'k': // Move up (Vim-style)
                highlight = (highlight == 0) ? num_choices - 1 : highlight - 1;
                break;
            case 'j': // Move down (Vim-style)
                highlight = (highlight == num_choices - 1) ? 0 : highlight + 1;
                break;
            case 10: // Enter key is pressed
                choice = highlight;
                // Clear the previously selected option to avoid lingering text
                mvprintw(row - 7, col - 28, "Selected: %s", menu_items[choice]);
                // Load the configuration file contents only when "Configuration" is selected
                if (choice == 0) {
                    free(config_contents); // Free previously allocated memory
                    config_contents = read_config_file(CONFIG_FILE_PATH); // Load new config contents
                }
                // Execute 'obs list' command when "View vault" is selected
                else if (choice == 1) {
                    free(vault_contents); // Free previously allocated memory
                    vault_contents = run_obs_list(); // Load new vault contents
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
                free(config_contents); // Free allocated memory
                free(vault_contents); // Free vault contents before exit
                endwin();
                return 0;
        }
    }

    // End ncurses mode
    free(config_contents); // Free allocated memory before exiting
    free(vault_contents); // Free vault contents before exit
    endwin();
    return 0;
}


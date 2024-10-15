#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Prototype of run_sil_add function
char* run_sil_add();

int main() {
    char* result = run_sil_add();  // Call the function

    if (result == NULL) {
        printf("run_sil_add() returned NULL. No data received or an error occurred.\n");
    } else {
        // Print the output if it is not null
        printf("run_sil_add() returned the following data:\n");
        printf("%s\n", result);

        // Free the allocated memory
        free(result);
    }

    return 0;
}

// The run_sil_add function (same as the one provided)
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


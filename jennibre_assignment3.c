#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>

#define PREFIX "movies_"
#define EXTENSION ".csv"
#define ONID "jennibre"
#define MAX_TITLE_LENGTH 256

void display_main_menu();
void display_file_selection_menu();
char* find_largest_or_smallest_file(int find_largest);
char* get_user_file(int *valid);
void process_file(const char *filename);
void create_directory_and_process_data(const char *filename);
int is_movies_file(const char *filename);
void clear_input_buffer();
void set_file_permissions(const char *file_path, mode_t mode);
char* trim_whitespace(char *str);
int is_valid_year(int year);

int main() {
    int choice;
    while (1) {
        display_main_menu();
        scanf("%d", &choice);
        clear_input_buffer();
        if (choice == 1) {
            int file_choice;
            while (1) {
                display_file_selection_menu();
                scanf("%d", &file_choice);
                clear_input_buffer();
                char *selected_file = NULL;
                int valid = 1;

                if (file_choice == 1) {
                    selected_file = find_largest_or_smallest_file(1);
                } else if (file_choice == 2) {
                    selected_file = find_largest_or_smallest_file(0);
                } else if (file_choice == 3) {
                    selected_file = get_user_file(&valid);
                    if (!valid) continue;
                } else {
                    printf("You entered an incorrect choice. Try again.\n");
                    continue;
                }

                if (selected_file) {
                    printf("Now processing the chosen file named %s\n", selected_file);
                    process_file(selected_file);
                    free(selected_file);
                    break;
                }
            }
        } else if (choice == 2) {
            exit(0);
        } else {
            printf("You entered an incorrect choice. Try again.\n");
        }
    }
    return 0;
}

void display_main_menu() {
    printf("\n1. Select file to process\n");
    printf("2. Exit the program\n");
    printf("\nEnter a choice 1 or 2: ");
}

void display_file_selection_menu() {
    printf("\nWhich file you want to process?\n");
    printf("Enter 1 to pick the largest file\n");
    printf("Enter 2 to pick the smallest file\n");
    printf("Enter 3 to specify the name of a file\n");
    printf("\nEnter a choice from 1 to 3: ");
}

char* find_largest_or_smallest_file(int find_largest) {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Unable to open directory");
        return NULL;
    }

    struct dirent *entry;
    struct stat file_stat;
    char *selected_file = NULL;
    long selected_size = find_largest ? 0 : LONG_MAX;

    while ((entry = readdir(dir)) != NULL) {
        if (is_movies_file(entry->d_name)) {
            if (stat(entry->d_name, &file_stat) == 0) {
                if ((find_largest && file_stat.st_size > selected_size) || 
                    (!find_largest && file_stat.st_size < selected_size)) {
                    selected_size = file_stat.st_size;
                    free(selected_file);
                    selected_file = strdup(entry->d_name);
                }
            } else {
                perror("Error getting file stats");
            }
        }
    }
    closedir(dir);

    if (!selected_file) {
        printf("No valid movies_*.csv files found.\n");
    }

    return selected_file;
}

char* get_user_file(int *valid) {
    char filename[256];
    printf("Enter the complete file name: ");
    scanf("%255s", filename);
    clear_input_buffer();

    if (access(filename, F_OK) == 0) {
        *valid = 1;
        return strdup(filename);
    } else {
        printf("The file %s was not found. Try again\n", filename);
        *valid = 0;
        return NULL;
    }
}

void process_file(const char *filename) {
    create_directory_and_process_data(filename);
}

void create_directory_and_process_data(const char *filename) {
    char directory_name[256];
    srand(time(NULL));
    int random_number = rand() % 100000;
    snprintf(directory_name, sizeof(directory_name), "%s.movies.%d", ONID, random_number);

    // Create directory with permissions 0755
    if (mkdir(directory_name, 0755) == 0) {
        printf("Created directory with name %s\n", directory_name);
    } else {
        perror("Error creating directory");
        return;
    }

    // Explicitly set directory permissions to 0755
    set_file_permissions(directory_name, 0755);

    // Open CSV file
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening CSV file");
        return;
    }

    char line[512];
    int header_skipped = 0;

    while (fgets(line, sizeof(line), file)) {
        if (!header_skipped) {  // Skip header
            header_skipped = 1;
            continue;
        }

        char title[MAX_TITLE_LENGTH];
        char year_str[16];
        int year;

        // Use strtok to handle cases where the title contains commas
        char *token = strtok(line, ",");
        if (token) {
            strncpy(title, trim_whitespace(token), MAX_TITLE_LENGTH - 1);
            title[MAX_TITLE_LENGTH - 1] = '\0';

            token = strtok(NULL, ",");
            if (token) {
                strncpy(year_str, trim_whitespace(token), sizeof(year_str) - 1);
                year_str[sizeof(year_str) - 1] = '\0';
                year = atoi(year_str);

                // Validate year
                if (!is_valid_year(year)) continue;

                char year_filename[512];
                snprintf(year_filename, sizeof(year_filename), "%s/%d.txt", directory_name, year);

                // Append movie to the corresponding year file
                FILE *year_file = fopen(year_filename, "a");
                if (year_file) {
                    fprintf(year_file, "%s\n", title);
                    fclose(year_file);

                    // Set file permissions to 0644
                    set_file_permissions(year_filename, 0644);
                } else {
                    perror("Error creating file");
                }
            }
        }
    }
    fclose(file);
}

int is_movies_file(const char *filename) {
    size_t len = strlen(filename);
    return (strncmp(filename, PREFIX, strlen(PREFIX)) == 0 &&
            len > strlen(EXTENSION) &&
            strcmp(filename + len - strlen(EXTENSION), EXTENSION) == 0);
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void set_file_permissions(const char *file_path, mode_t mode) {
    if (chmod(file_path, mode) != 0) {
        perror("Error setting file permissions");
    }
}

char* trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;  // All spaces

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

int is_valid_year(int year) {
    return (year >= 1900 && year <= 2025);  // Only allow realistic movie years
}

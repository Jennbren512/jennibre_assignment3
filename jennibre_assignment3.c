#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

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
                } else {
                    printf("No valid file was selected.\n");
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

    printf("\nScanning directory for CSV files...\n");
    
    while ((entry = readdir(dir)) != NULL) {
        printf("Checking file: %s\n", entry->d_name);

        if (entry->d_name[0] == '.') continue;

        if (is_movies_file(entry->d_name)) {
            printf("Matched CSV file: %s\n", entry->d_name);

            if (stat(entry->d_name, &file_stat) == 0) {
                printf("File size: %ld bytes\n", file_stat.st_size);

                if ((find_largest && file_stat.st_size > selected_size) ||
                    (!find_largest && file_stat.st_size < selected_size)) {
                    selected_size = file_stat.st_size;
                    free(selected_file);
                    selected_file = strdup(entry->d_name);
                    printf("New %s file found: %s (%ld bytes)\n", 
                           find_largest ? "largest" : "smallest", selected_file, selected_size);
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

    printf("Checking if file %s exists...\n", filename);

    if (access(filename, F_OK) == 0) {
        printf("File found: %s\n", filename);
        *valid = 1;
        return strdup(filename);
    } else {
        printf("The file %s was not found. Try again\n", filename);
        *valid = 0;
        return NULL;
    }
}

void process_file(const char *filename) {
    printf("Processing file: %s\n", filename);
    create_directory_and_process_data(filename);
}

void create_directory_and_process_data(const char *filename) {
    char directory_name[256];
    srand(time(NULL));
    int random_number = rand() % 100000;
    snprintf(directory_name, sizeof(directory_name), "%s.movies.%d", ONID, random_number);

    if (mkdir(directory_name, 0750) == 0) {
        printf("Created directory with name %s\n", directory_name);
    } else {
        perror("Error creating directory");
        return;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening CSV file");
        return;
    }

    char line[512];
    int header_skipped = 0;

    while (fgets(line, sizeof(line), file)) {
        if (!header_skipped) {
            header_skipped = 1;
            continue;
        }

        char title[MAX_TITLE_LENGTH];
        int year;
        if (sscanf(line, "%255[^,],%d", title, &year) == 2) {
            char year_filename[512];
            snprintf(year_filename, sizeof(year_filename), "%s/%d.txt", directory_name, year);

            FILE *year_file = fopen(year_filename, "a");
            if (year_file) {
                fprintf(year_file, "%s\n", title);
                fclose(year_file);
                chmod(year_filename, 0640);
            } else {
                perror("Error creating file");
            }
        }
    }
    fclose(file);
}

int is_movies_file(const char *filename) {
    size_t len = strlen(filename);
    int match = (strncmp(filename, PREFIX, strlen(PREFIX)) == 0 &&
                 len > strlen(EXTENSION) &&
                 strcmp(filename + len - strlen(EXTENSION), EXTENSION) == 0);
    
    printf("Checking if %s is a valid movie CSV file: %s\n", filename, match ? "YES" : "NO");
    return match;
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

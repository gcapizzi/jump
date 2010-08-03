/*
 * Jump, a bookmarking system for the bash shell.
 * Copyright (c) 2010 Giuseppe Capizzi
 * mailto: g.capizzi@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_SIZE      64
#define PATH_SIZE     256

char bookmarks_filename[] = ".jumprc";

typedef struct bookmark_struct bookmark;
struct bookmark_struct {
    char *name;
    char *path;
    bookmark *next;
};

bookmark *bookmarks = NULL;

void exit_with_error(const char *message) {
    printf("*** ERROR: %s\n", message);
    exit(1);
}

void print_usage() {
    printf("jump to [bookmark]    Jumps to the directory pointed by [bookmark]\n");
    printf("jump add [bookmark]   Saves the current directory in [bookmark]\n");
    printf("jump del [bookmark]   Deletes [bookmark]\n");
    printf("jump list             Prints the list of all saved bookmarks\n");
    printf("jump help             Displays this message\n");
}

// Returns the current working directory.
char *cwd() {
    char *working_dir;
    FILE *fp;

    working_dir = calloc(PATH_SIZE, sizeof(char));

    fp = popen("pwd", "r");
    if (fp == NULL) {
        exit_with_error("failed to run pwd.");
    }
    fgets(working_dir, PATH_SIZE, fp);
    pclose(fp);

    return working_dir;
}

// Returns the path of the bookmarks file in the user home directory.
char *get_bookmarks_filepath() {
    char *homedir = getenv("HOME");
    int filepath_len = strlen(homedir) + strlen(bookmarks_filename) + 1;

    char *bookmarks_filepath;
    bookmarks_filepath = calloc(filepath_len, sizeof(char));
    strcpy(bookmarks_filepath, homedir);
    strcat(bookmarks_filepath, "/");
    strcat(bookmarks_filepath, bookmarks_filename);
    
    return bookmarks_filepath;
}

// Adds a bookmark to the list.
bookmark *add_bookmark(char *name, char *path) {
    bookmark *b;

    // create bookmark node
    b = malloc(sizeof(bookmark));
    b->name = strdup(name);
    b->path = strdup(path);
    b->next = NULL;

    // add bookmark to the list
    if (bookmarks == NULL) {
        bookmarks = b;
    } else {
        bookmark *p = bookmarks;

        // find the last but one node
        while (p->next != NULL) {
            p = p->next;
        }

        // add the node to the list
        p->next = b;
    }

    return b;
}

// Finds a bookmark in the list.
bookmark *find_bookmark(char *name) {
    bookmark *p = bookmarks;

    while (p != NULL && strcmp(p->name, name) != 0) {
        p = p->next;
    }

    return p;
}

// Creates a new bookmark if it does not exist. If it does already exist,
// replaces its path with the new one.
bookmark *new_bookmark(char *name, char *path) {
    bookmark *p;

    if ((p = find_bookmark(name)) != NULL) {
        strcpy(p->path, path);
    } else {
        p = add_bookmark(name, path);
    }

    return p;
}

// Deletes a bookmark from the list.
void delete_bookmark(char *name) {
    bookmark *d;

    // the record is the first in the list
    if (bookmarks != NULL && strcmp(bookmarks->name, name) == 0) {
        d = bookmarks;
        bookmarks = bookmarks->next;
        free(d);
    } else {
        bookmark *p = bookmarks;
        
        // find the previous node
        while (p != NULL && p->next != NULL && strcmp(p->next->name, name) != 0) {
            p = p->next;
        }

        if (p->next != NULL) {
            d = p->next;
            p->next = d->next;
            free(d);
        } else {
            exit_with_error("no bookmark found");
        }
    }
}

// Saves the bookmarks list in the bookmarks file.
void save_bookmarks() {
    FILE *f;
    bookmark *p = bookmarks;

    if ((f = fopen(get_bookmarks_filepath(), "w")) != NULL) {
        while (p != NULL) {
            fprintf(f, "%s\t%s\n", p->name, p->path);
            p = p->next;
        }
    } else {
        exit_with_error("can't save configuration file");
    }
}

// Loads the bookmarks list from the bookmarks file.
void load_bookmarks() {
    FILE *f;
    char *name, *path;
    name = calloc(NAME_SIZE, sizeof(char));
    path = calloc(PATH_SIZE, sizeof(char));

    if ((f = fopen(get_bookmarks_filepath(), "r")) != NULL) {
        while (fscanf(f, "%s\t%s", name, path) != EOF) {
            add_bookmark(name, path);
        }
    }
}

// Prints the bookmarks list.
void print_bookmarks() {
    bookmark *p = bookmarks;
    int i;

    if (p != NULL) {
        printf("Bookmark             Path\n");
        printf("---------------------------------------------------------------------------\n");
        while (p != NULL) {
            printf("%s", p->name);
            for (i = 0; i < (20 - strlen(p->name)); i++) {
                printf(" ");
            }
            printf(" %s\n", p->path);
            p = p->next;
        }
        
    } else {
        printf("No bookmarks saved.\n");
    }
}

// Expands paths that could start with a bookmark (e.g. [bookmark]/sub/path)
char *expand_path(char *path_with_bookmark) {
    const char *subpath = strchr(path_with_bookmark, '/');

    if (subpath != NULL) {
        // the path contains a subpath (e.g. [bookmark]/sub/path)
        if (subpath == path_with_bookmark) {
            // the path is an absolute path (no bookmark, e.g. /absolute/path)
            char *pwb = strdup(path_with_bookmark);
            return pwb;
        } else {
            // the path is composed of a bookmark and a subpath

            // copy the bookmark part in a variable
            char *b;
            b = calloc(subpath - path_with_bookmark, sizeof(char));
            strncpy(b, path_with_bookmark, (int)(subpath - path_with_bookmark));

            // find the corresponding path
            char *b_path;
            b_path = find_bookmark(b)->path;

            // concatenate the bookmark path with the subpath
            char *expanded_path;
            expanded_path = calloc(strlen(b_path) + strlen(subpath), sizeof(char));
            strcpy(expanded_path, b_path);
            strcat(expanded_path, subpath);

            return expanded_path;
        }
    } else {
        // the path is just a bookmark
        return find_bookmark(path_with_bookmark)->path;
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if(strcmp(argv[1], "list") == 0) {
            load_bookmarks();
            print_bookmarks();
        } else if(strcmp(argv[1], "help") == 0) {
            print_usage();
        } else if(strcmp(argv[1], "to") == 0 && argc > 2) {
            load_bookmarks();
            
            char *path;
            path = expand_path(argv[2]);
            if (path != NULL) {
                printf("%s", path);
            }
        } else if(strcmp(argv[1], "add") == 0 && argc > 2) {
            load_bookmarks();
            new_bookmark(argv[2], cwd());
            save_bookmarks();
        } else if(strcmp(argv[1], "del") == 0 && argc > 2) {
            load_bookmarks();
            delete_bookmark(argv[2]);
            save_bookmarks();
        } else {
            print_usage();
            exit(1);
        }
    } else { // wrong number of arguments
        print_usage();
        exit(1);
    }

    return 0;
}

/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*/
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MODE_DEFAULT    0
#define MODE_LONG       1
#define MODE_HORIZONTAL 2
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

extern int errno;

void do_ls(const char *dir, int mode);
void print_long(const char *dir, const char *name);
void mode_to_string(mode_t mode, char str[]);
int get_terminal_width(void);
int compare_names(const void *a, const void *b);
void print_columns(const char *dir, char **names, int count, int maxlen);
void print_horizontal(const char *dir, char **names, int count, int maxlen);
void print_colored(const char *name, mode_t mode);
mode_t get_mode(const char *dir, const char *name);

int main(int argc, char *argv[])
{
    int opt;
    int mode = MODE_DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            mode = MODE_LONG;
            break;
        case 'x':
            mode = MODE_HORIZONTAL;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [dir...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], mode);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int mode)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char **names = NULL;
    int count = 0;
    int capacity = 0;
    int maxlen = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity)
        {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            names = realloc(names, capacity * sizeof(char *));
            if (names == NULL)
            {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }

        names[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > maxlen)
            maxlen = len;
        count++;
    }

    if (errno != 0)
        perror("readdir failed");
    closedir(dp);
        qsort(names, count, sizeof(char *), compare_names);
       if (mode == MODE_LONG)
    {
        for (int i = 0; i < count; i++)
            print_long(dir, names[i]);
    }
    else if (mode == MODE_HORIZONTAL)
    {
        print_horizontal(dir, names, count, maxlen);    }
    else
    {
        print_columns(dir, names, count, maxlen);    }

    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}
void mode_to_string(mode_t mode, char str[])
{
    strcpy(str, "----------");

    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';
    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';
    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';

    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';
}

void print_long(const char *dir, const char *name)
{
    char path[1024];
    struct stat info;
    char perms[11];

    snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (lstat(path, &info) == -1)
    {
        perror(path);
        return;
    }

    mode_to_string(info.st_mode, perms);

    struct passwd *pw = getpwuid(info.st_uid);
    struct group *gr = getgrgid(info.st_gid);

    char *t = ctime(&info.st_mtime);

       printf("%s %2ld %-8s %-8s %8ld %.12s ",
           perms,
           (long)info.st_nlink,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           (long)info.st_size,
           t + 4);
    print_colored(name, info.st_mode);
    printf("\n");
}
int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return 80;
    return w.ws_col;
}

void print_columns(const char *dir, char **names, int count, int maxlen)
{
    if (count == 0)
        return;

    int width = get_terminal_width();
    int col_width = maxlen + 2;

    int cols = width / col_width;
    if (cols < 1)
        cols = 1;

    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int i = r + c * rows;
            if (i >= count)
                break;
            print_colored(names[i], get_mode(dir, names[i]));
            printf("%*s", col_width - (int)strlen(names[i]), "");
        }
        printf("\n");
    }
}

void print_horizontal(const char *dir, char **names, int count, int maxlen)
{
    int width = get_terminal_width();
    int col_width = maxlen + 2;
    int pos = 0;

    for (int i = 0; i < count; i++)
    {
        if (pos + col_width > width && pos > 0)
        {
            printf("\n");
            pos = 0;
        }
        print_colored(names[i], get_mode(dir, names[i]));
        printf("%*s", col_width - (int)strlen(names[i]), "");
        pos += col_width;
    }

    if (count > 0)
        printf("\n");
}
int compare_names(const void *a, const void *b)
{
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}
mode_t get_mode(const char *dir, const char *name)
{
    char path[1024];
    struct stat info;

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    if (lstat(path, &info) == -1)
        return 0;
    return info.st_mode;
}

void print_colored(const char *name, mode_t mode)
{
    const char *color = NULL;

    if (S_ISLNK(mode))
        color = COLOR_PINK;
    else if (S_ISDIR(mode))
        color = COLOR_BLUE;
    else if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISSOCK(mode) || S_ISFIFO(mode))
        color = COLOR_REVERSE;
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
        color = COLOR_RED;
    else if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        color = COLOR_GREEN;

    if (color)
        printf("%s%s%s", color, name, COLOR_RESET);
    else
        printf("%s", name);
}
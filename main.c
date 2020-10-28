#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <unistd.h>


#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>

struct Options
{
    int l, R, t, h, help, ver;
    char dir[200];
};

char *size_s(float f);

char *human_size(unsigned int d);

int compare_files(const struct dirent **a, const struct dirent **b);

void print_line(const struct dirent **dp, const struct stat *statbuf);
void list_dir();

char *human_size(unsigned int d);

void read_option(const char * s);

struct Options opt = { 0, 0, 0, 0, 0, 0, "./" };

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (*(argv + i)[0] == '-')
            read_option(*(argv + i));
        else {
            //if (*(argv + i)[0] == '.') {
            //}
            //else {
                strcpy(opt.dir, *(argv + i));
                strcat(opt.dir, "/");
            //}
            printf("DIR = %s\n", opt.dir);
        }
        if (opt.ver) {
            printf("Wersja 1\n");
            printf("Lukasz Kadracki\n");
            return 0;
        }
        else if (opt.help) {
            printf("Uzycie ./ls [OPTION]... [FILE]\n");
            printf("-h czytelne rozmiary plikow\n");
            printf("-l wyswietlanie wlasciwosci plikow\n");
            printf("-R rekurencyjne wyswietlanie podkatalogow\n");
            printf("-t sortowanie po dacie modyfikacji\n");
            printf("--help wyswietla dostepne opcje\n");
            printf("--version wyswietla numer wersji\n");
            return 0;
        }
    }

    list_dir();
}

char *size_s(float f)
{
    char * output = calloc(20, sizeof(char));
    snprintf(output, 20, "%.1f", f);
    return output;
}

int compare_files(const struct dirent **a, const struct dirent **b)
{
    struct stat stat_a, stat_b;
    char aname[200] = "", bname[200] = ""; 
    strcat(strcat(aname, opt.dir), (*a)->d_name);
    strcat(strcat(bname, opt.dir), (*b)->d_name);
    stat(aname, &stat_a);
    stat(bname, &stat_b);
    if (stat_a.st_atime < stat_b.st_atime)
        return 1;
    else
        return 0;
}

void list_dir()
{
    DIR *dir;

    if ((dir = opendir(opt.dir)) == NULL) {
        printf("Nie mozna otworzyc katalogu %s\n", opt.dir);
        return;
    }

    int r_count = 0;
    char r_names[300][100];

    if (opt.t) {
        struct dirent **dirs;
        int n = scandir(opt.dir, &dirs, NULL, compare_files);
        if (n < 0)
            printf("Nie mozna otworzyc katalogu\n");
        else {
            struct stat statbuf;
            for (int i = 0; i < n; i++) {
                char name[200] = "";
                strcat(strcat(name, opt.dir), dirs[i]->d_name);
                if (stat(name, &statbuf) == -1)
                    continue;
                if (dirs[i]->d_name[0] == '.')
                    continue;
                print_line(&dirs[i], &statbuf);
                if (opt.R && S_ISDIR(statbuf.st_mode)) {
                    strcpy(r_names[r_count], dirs[i]->d_name);
                    r_count++;
                }
            }
            free(dirs);
        }
    }
    else {
        struct dirent *dp;
        struct stat statbuf;
        while ((dp = readdir(dir)) != NULL) {
            char name[200] = "";
            strcat(strcat(name, opt.dir), dp->d_name);
            if (stat(name, &statbuf) == -1) {
                printf("%s: %s\n", name, strerror(errno));
                continue;
            }
            if (dp->d_name[0] == '.')
                continue;
            print_line(&dp, &statbuf);
            if (opt.R && S_ISDIR(statbuf.st_mode)) {
                strcpy(r_names[r_count], dp->d_name);
                r_count++;
            }
        }
    }

    if (opt.R) {
        char curr_dir[200] = "";
        strcpy(curr_dir, opt.dir);
        for (int i = 0; i < r_count; i++) {
            printf("\n%s:\n", r_names[i]);
            strcat(strcat(opt.dir, r_names[i]), "/");
            list_dir();
           
            strcpy(opt.dir, curr_dir);
        }
    }
    closedir(dir);
}

void print_line(const struct dirent **dp, const struct stat *statbuf)
{
    printf((S_ISDIR(statbuf->st_mode)) ? "d" : "-");
    printf((statbuf->st_mode & S_IRUSR) ? "r" : "-");
    printf((statbuf->st_mode & S_IWUSR) ? "w" : "-");
    printf((statbuf->st_mode & S_IXUSR) ? "x" : "-");
    printf((statbuf->st_mode & S_IRGRP) ? "r" : "-");
    printf((statbuf->st_mode & S_IWGRP) ? "w" : "-");
    printf((statbuf->st_mode & S_IXGRP) ? "x" : "-");
    printf((statbuf->st_mode & S_IROTH) ? "r" : "-");
    printf((statbuf->st_mode & S_IWOTH) ? "w" : "-");
    printf((statbuf->st_mode & S_IXOTH) ? "x" : "-");

    printf(" %d", statbuf->st_nlink);

    struct passwd *pwd = getpwuid(statbuf->st_uid);
    printf(" %s", pwd->pw_name);

    struct group *grp = getgrgid(statbuf->st_gid);
    printf(" %s", grp->gr_name);

    if (opt.h) {
        char * size = human_size((unsigned int) statbuf->st_size);
        printf("  %s", size);
        free(size);
    }
    else
        printf(" %u", statbuf->st_size);

    char buff[20];
    strftime(buff, sizeof(buff), "%H:%M %b %d %Y", gmtime(&statbuf->st_atime));
    printf(" %s", buff);
    printf(" %s\n", (*dp)->d_name);
}

char *human_size(unsigned int d)
{
    if (d >= 1000000)
        return strcat(size_s((float)d / 1000000), "T");
    else if (d >= 100000)
        return strcat(size_s((float)d / 100000), "G");
    else if (d >= 10000)
        return strcat(size_s((float)d / 10000), "M");
    else if (d >= 1000)
        return strcat(size_s((float)d / 1000), "K");        
    else
        return size_s((float)d);
}



void read_option(const char * s)
{
    for (int i = 1; i < strlen(s); i++) {
        if (s[i] == 'l')
            opt.l = 1;
        else if (s[i] == 'R')
            opt.R = 1;
        else if (s[i] == 't')
            opt.t = 1;
        else if (s[i] == 'h')
            opt.h = 1;
        else if (s[i] == '-') {
            if (strcmp(s, "--version") == 0)
                opt.ver = 1;
            else if (strcmp(s, "--help") == 0)
                opt.help = 1;
        }
    }    
}

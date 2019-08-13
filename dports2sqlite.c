#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <dirent.h>	// opendir,closedir, readdir
#include <string.h>	// strcat, strncpy
#include <unistd.h>	// usleep
//#include <sys/types.h>
//#include <sys/stat.h>

#include "thpool.h"

// defines
#define NAME_LEN 25
#define DIRNAME_LEN 25
#define PATH_LEN 255

// globals
int num_threads = 4;
char *dports_dir = "/build/home/dkgroot/dports/";
threadpool pool;
sqlite3 *db;

typedef struct cat {
    char name[NAME_LEN];
    char dirname[DIRNAME_LEN];
} cat_t;

typedef struct port {
    cat_t *cat;
    char name[NAME_LEN];
    char dirname[DIRNAME_LEN];
} port_t;

void process_makefile()
{
}

void process_description()
{
}

void *process_port(void *args)
{
    port_t *port = args;
    printf("Processing port:%s of cat%s\n", port->name, port->cat->name);
    
    usleep(1000);
    
    free(port);
    return NULL;
}

void process_cat(cat_t *cat)
{
    struct dirent *portdir = NULL;
    printf ("- category: %s\n", cat->name);
    char cat_dir_str[PATH_LEN]="";
    strcat(cat_dir_str, dports_dir);
    strcat(cat_dir_str, cat->dirname);

    DIR *cat_dir = opendir(cat_dir_str);
    if (cat_dir) {
        while((portdir = readdir(cat_dir)))
        {
            if (portdir->d_type == DT_DIR && portdir->d_name[0] != '.') {
                printf ("   - port: %s\n", portdir->d_name);
                port_t *port = calloc(1, sizeof(port_t));			// freed by process_port
                if (port) {
                    port->cat = cat;
                    strncpy(port->name, portdir->d_name, sizeof port->name - 1);
                    strncpy(port->dirname, portdir->d_name, sizeof port->name - 1);
                    thpool_add_work(pool, (void *)process_port, (void *)port);
                }
            }
        }
        closedir(cat_dir);
    }
}

void process_dports()
{
    struct dirent *catdir = NULL;    
    DIR *ports = opendir(dports_dir);
    if (ports) {
        while((catdir = readdir(ports))){
            if (catdir->d_type == DT_DIR && catdir->d_name[0] != '.') {
                cat_t *cat = calloc(1, sizeof(cat_t));				// freed auto at program exit
                strncpy(cat->name, catdir->d_name, sizeof cat->name - 1);
                strncpy(cat->dirname, catdir->d_name, sizeof cat->dirname - 1);
                process_cat(cat);
            }
        }
        closedir(ports);
    }
}

int main(int argc, char** argv)
{
    printf("Generating dport sqlite3 db\n");

    int rc;
    rc = sqlite3_open("index.db", &db);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    
    pool = thpool_init(num_threads);
    if (!pool) {
        fprintf(stderr, "Can't create threadpool\n");
        goto close_db;
    }
    
    process_dports();

    thpool_wait(pool);
    thpool_destroy(pool);

close_db:
    sqlite3_close(db);
    return 0;
}
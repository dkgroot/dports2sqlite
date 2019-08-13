#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <dirent.h>	// opendir,closedir, readdir
#include <string.h>	// strcat, strncpy
#include <unistd.h>	// usleep
#include <stdarg.h>	// va_start, va_end
//#include <sys/types.h>
//#include <sys/stat.h>

#include "thpool.h"

// defines
#define NAME_LEN 25
#define DIRNAME_LEN 25
#define PATH_LEN 255

// debug logging
#if defined(DEBUG)
#define DEBUG_LOG(...) fprintf(stdout, VA_ARGS)
#else
#define DEBUG_LOG(...)
#endif

// globals
int num_threads = 4;
char *dports_dir = "/build/home/dkgroot/dports/";
threadpool pool;
sqlite3 *db;

typedef struct cat {
    int id;
    char name[NAME_LEN];
    char dirname[DIRNAME_LEN];
} cat_t;

typedef struct port {
    int id;
    cat_t *cat;
    int cat_id;
    char name[NAME_LEN];
    char dirname[DIRNAME_LEN];
    char *makefile;
    char *pkg_descr;
} port_t;

char *copy_file_to_string(char * filename, char *buffer)
{
    size_t length;
    size_t read_length;
    FILE * f = fopen (filename, "rb");
    if (!f)
    {
        return NULL;
    }
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = malloc (length+1);
    if (buffer)
    {
        buffer[length] = '\0';
        read_length = fread (buffer, 1, length, f);
        if (length != read_length) {
            return NULL;
        }
    }
    fclose (f);
    return buffer;
}

void process_makefile()
{
}

void process_description()
{
}

void *process_port(void *args)
{
    port_t *port = args;
    DEBUG_LOG("Processing port:%s of cat%s\n", port->name, port->cat->name);

    //usleep(1000);

    char makefile[PATH_LEN]; 
    snprintf(makefile, PATH_LEN, "%s/%s/%s/%s", dports_dir, port->cat->dirname, port->dirname, "Makefile");
    copy_file_to_string(makefile, port->makefile);

    char pkgdescr[PATH_LEN]; 
    snprintf(pkgdescr, PATH_LEN, "%s/%s/%s/%s", dports_dir, port->cat->dirname, port->dirname, "pkg-descr");
    copy_file_to_string(pkgdescr, port->pkg_descr);
    
    free(port->makefile);
    free(port->pkg_descr);
    free(port);
    return NULL;
}

int insert_into_db(const char *format, ...)
{
    int rc;
    char *err_msg = 0;
    va_list arg;
    char sql[255];

    va_start (arg, format);
    vsnprintf (sql, sizeof sql - 1, format, arg);
    va_end (arg);

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}

void process_cat(cat_t *cat)
{
    struct dirent *portdir = NULL;
    DEBUG_LOG("- category: %s\n", cat->name);
    int cat_id = insert_into_db("INSERT INTO Category(Name, DirName) VALUES('%s', '%s');", cat->dirname, cat->dirname);
    if (cat_id < 0) return;
    cat->id = cat_id;
    
    char cat_dir_str[PATH_LEN]="";
    strcat(cat_dir_str, dports_dir);
    strcat(cat_dir_str, cat->dirname);

    DIR *cat_dir = opendir(cat_dir_str);
    if (cat_dir) {
        while((portdir = readdir(cat_dir)))
        {
            if (portdir->d_type == DT_DIR && portdir->d_name[0] != '.') {
                int port_id = insert_into_db("INSERT INTO Port(PkgName, DirName, MaintainerId, CatId) VALUES('%s', '%s', 0, %d);", portdir->d_name, portdir->d_name, cat_id);
                if (port_id < 0) return; 
                DEBUG_LOG("   - port: %s\n", portdir->d_name);
                port_t *port = calloc(1, sizeof(port_t));			// freed by process_port
                if (port) {
                    port->id = port_id;
                    port->cat = cat;
                    port->cat_id = cat_id;
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

int setup_sqlite()
{
    int rc;
    char *err_msg = 0;
    char *sql = "DROP TABLE IF EXISTS Category;"
                "DROP TABLE IF EXISTS Port;"
                "DROP TABLE IF EXISTS Deps;"
                "DROP TABLE IF EXISTS Maintainer;"
                "DROP TABLE IF EXISTS DepTypes;"
                "CREATE TABLE Category(Id INT PRIMARY KEY, Name TEXT, DirName TEXT);"
                "CREATE TABLE Maintainer(Id INT PRIMARY KEY, Name TEXT, Email TEXT);"
                "CREATE TABLE Port(Id INT PRIMARY KEY, PkgName TEXT, DirName TEXT, Path TEXT, Prefix TEXT Comment TEXT, Description TEXT,"
                    "MaintainerId INT NOT NULL REFERENCES Maintainer(Id), CatId INT NOT NULL REFERENCES Category(Id));"
                "CREATE TABLE DepTypes(Type CHAR(1) PRIMARY KEY, Name TEXT);"
                "INSERT INTO DepTypes VALUES('B', 'Build');"
                "INSERT INTO DepTypes VALUES('R', 'Run');"
                "INSERT INTO DepTypes VALUES('E', 'Extract');"
                "INSERT INTO DepTypes VALUES('P', 'Patch');"
                "INSERT INTO DepTypes VALUES('F', 'Fetch');"
                "CREATE TABLE Deps(PortId INT NOT NULL REFERENCES Port(Id), DepPortId INT NOT NULL REFERENCES Port(Id), DepType CHAR(1) NOT NULL DEFAULT ('B') REFERENCES DepTypes(Type))";
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    return 0;
}


int main(int argc, char** argv)
{
    DEBUG_LOG("Generating dport sqlite3 db\n");

    int rc;
    rc = sqlite3_open("index.db", &db);
    if (rc != SQLITE_OK){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    if (setup_sqlite()) {
        goto close_db;
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
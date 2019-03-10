#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

//Custom functions:
int show_help();
int yn_question( char* );

//Action functions:
int server_add( char*, char*, int*, char* );
int server_remove( int* );
int server_search( char* );
int server_open( int* );

//Database callbacks:
static int server_search_callback(void*, int, char**, char**);
static int server_open_callback(void*, int, char**, char**);

//Database functions:
int db_connect();
int db_close();

sqlite3 *db;

int main( int argc, char *argv[] )
{
    if( argc < 3 )
    {
        show_help();
        return 0;
    }

    //Remove row:
    if( strcmp(argv[1],"remove") == 0 )
    {
        if( argc != 3 )
        {
            printf("Error: Invalid argument count\n");
            show_help();
            return 0;
        }
        printf("Remove %s\n", argv[2]);
        int id = atoi(argv[2]);
        int res = server_remove( &id );
        if( res == 0 )
            return 1;
    }

    //Add row:
    if( strcmp(argv[1],"add") == 0 )
    {
        if( argc != 6 )
        {
            printf("Error: Invalid argument count\n");
            show_help();
            return 0;
        }
        //name, host, port, protocol
        printf("Add %s %s %s %s\n", argv[2], argv[3], argv[4], argv[5]);
        int port = atoi(argv[4]);
        int res = server_add( argv[2], argv[3], &port, argv[5] );
        if( res == 0 )
            return 1;
    }

    //Search:
    if( strcmp(argv[1],"search") == 0 )
    {
        if( argc != 3 )
        {
            printf("Error: Invalid argument count\n");
            show_help();
            return 0;
        }
        printf("\n\tSearching for %s:\n\t-----------------------------------------------------------\n", argv[2]);
        int res = server_search( argv[2] );
        printf("\t-----------------------------------------------------------\n\n");
        if( res == 0 )
            return 1;
    }

    //Open by Id:
    if( strcmp(argv[1],"o") == 0 )
    {
        if( argc != 3 )
        {
            printf("Error: Invalid argument count\n");
            show_help();
            return 0;
        }
        int id = atoi( argv[2] );
        int res = server_open( &id );
        if( res == 0 )
            return 1;
    }

    return 0;
}
/*
 *  Open server by server id
 */
int server_open( int *id )
{
    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf("SELECT id, name, host, port, protocol FROM srvl WHERE id='%q' LIMIT 0,1", id);

    int rc = sqlite3_exec(db, query, server_open_callback, 0, &err_msg);
    if (rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }
    db_close();
    return 1;
}
/*
 *  Open callback
 *  sqlite calls this function when the open-query is done
 *  NOTE: returns 0 on success!
 */
static int server_open_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    //ARGV: id, name, host, port, protocol
    if( argc > 0 && argv[4] != NULL )
    {
        if( strcmp(argv[4],"sftp") == 0 )
        {
            
        }
    }
    return 0;
}
/*
 *  Add server to database
 *  returns 1 on success
 *  returns 0 on fail
 *  @name Name of the server
 *  @host Ip or domain
 *  @port Server port
 *  @protocol Protocol to be used when opening with function server_open()
 */
int server_add( char *name, char *host, int *port, char *protocol )
{
    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf("INSERT INTO srvl('name','host','port','protocol') VALUES('%q', '%q', '%d', '%q')", name, host, *port, protocol);
    int rc = sqlite3_exec(db, query, 0, 0, &err_msg);
    if (rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }
    db_close();
    return 1;
}

/*
 *  Remove column
 *  returns 1 on success
 *  returns 0 on fail
 *  id the id of the column to be removed
 */
int server_remove( int *id )
{

    //Ask if this really is a good idea:
    if( yn_question("Do you really want to delete?") == 0 )
    {
        printf("Removal aborted\n");
        return 0;
    }

    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf("DELETE FROM srvl WHERE id='%d'",*id);
    int rc = sqlite3_exec(db, query, 0, 0, &err_msg);
    if (rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return 0;
    }
    db_close();
    return 1;
}

/*
 *  Search database by a string and display result
 */
int server_search( char *searchstring )
{
    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf("SELECT id, name, host, port, protocol " 
    "FROM srvl WHERE "
    "name LIKE '%%%q%%' OR "
    "host LIKE '%%%q%%' OR "
    "port LIKE '%%%q%%' OR "
    "protocol LIKE '%%%q%%' "
    "ORDER BY id ASC", searchstring, searchstring, searchstring, searchstring);

    int rc = sqlite3_exec(db, query, server_search_callback, 0, &err_msg);
    if (rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }
    db_close();
    return 1;
}

/*
 *  Search callback
 *  sqlite calls this function when the search-query is done
 *  NOTE: returns 0 on success!
 */
static int server_search_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    printf("\t%s. %s, %s, %s, %s\n", argv[0],argv[1],argv[2],argv[3],argv[4]);
    return 0;
}

/*
 *  Connect to databasefile:
 *  returns 1 on success
 *  returns 0 on fail
 */
int db_connect()
{
    //Connect to datbase and create one if it does not exist:
    int rc = sqlite3_open_v2("srvl.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    //Create a table if it doesnt exist:
    char *err_msg = 0;
    char *query = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS `srvl` ("
                                "`id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"
                                "`name`	TEXT,"
                                "`host`	TEXT,"
                                "`port`	INTEGER,"
                                "`protocol`	TEXT);");
    int rc2 = sqlite3_exec(db, query, 0, 0, &err_msg);
    if (rc2 != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }

    return 1;
}

/*
 * Close connection if db is not null
 */
int db_close()
{
    if( db != NULL )
        sqlite3_close(db);
}

/*
 *  Ask a question
 *  return 1 on YES
 *  return 0 on NO
 *  *question is the question to be asked
 */
int yn_question( char *question )
{
    int result=3;
    char answer;
    while( result == 3 )
    {
        printf("%s (y)es or (n)o: ", question);
        scanf(" %c", &answer);
        if( tolower(answer) == 'y' )
        {
            result = 1;
        }
        if( tolower(answer) == 'n' )
        {
            result = 0;
        }
    }
    
    return result;
}

/*
 *  Display help:
 */
 int show_help()
 {
     printf("\n====================================================================\n");
     printf("SERVER HELP\n");
     printf("====================================================================\n");
     printf("Add:\t\t server add <name> <host> <port> <protocol>\n");
     printf("--------------------------------------------------------------------\n");
     printf("Remove:\t\t server remove <id>\n");
     printf("--------------------------------------------------------------------\n");
     printf("Search:\t\t server search <search string>\n");
     printf("--------------------------------------------------------------------\n");
     printf("Open by id:\t server o <id>\n");
     printf("--------------------------------------------------------------------\n");
     printf("Open by name:\t server O <name>\n");
     printf("--------------------------------------------------------------------\n\n");
 }
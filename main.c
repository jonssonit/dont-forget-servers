#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

//Custom functions:
int show_help();
int yn_question( char* );

//Action functions:
int server_add( char*, char*, int*, char*, char* );
int server_remove( int* );
int server_search( char* );
int server_open( int* );
int server_list_all();

//Database callbacks:
static int server_search_callback( void*, int, char**, char** );
static int server_open_callback( void*, int, char**, char** );
static int server_list_all_callback( void*, int, char**, char** );

//Database functions:
int db_connect();
int db_close();

sqlite3 *db;

int main( int argc, char *argv[] )
{
	
    int validArg=0; //Is set to 1 if a argument is valid.
	
    if( argc < 2 )
    {
		
        show_help();
        return 0;
		
    } else if( strcmp(argv[1],"remove") == 0 ) {
		
		//Remove server from database:
        validArg = 1;
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
		
    } else if( strcmp(argv[1],"add") == 0 ) {
	
		//Add server
        validArg = 1;
        if( argc != 7 )
        {
            printf("Error: Invalid argument count\n");
            show_help();
            return 0;
        }

        //name, host, port, protocol, username
        printf("Add %s %s %s %s %s\n", argv[2], argv[3], argv[4], 
               argv[5], argv[6]);
        int port = atoi(argv[4]);
        int res = server_add( argv[2], argv[3], &port, argv[5], argv[6] );
        if( res == 0 )
            return 1;
	
    } else if( strcmp(argv[1],"search") == 0 ) {
		
		//Search for a server
        validArg = 1;
        if( argc != 3 )
        {
            printf("Error: Invalid argument count\n");
            show_help();
            return 0;
        }
        printf("\nSearching for %s:\n"
               "--------------------------------------------------------\n",
               argv[2]);
		
        int res = server_search( argv[2] );
        printf("-------------------------------"
		"-------------------------\n\n");
        if( res == 0 )
            return 1;
		
    } else if( strcmp(argv[1],"o") == 0 ) {
		
		//Open by database-id
        validArg = 1;
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
		
    } else if( strcmp(argv[1],"list") == 0 ) {
		
		//List all servers
        validArg = 1;
        if( argc != 2 )
        {
            printf("Error: Invalid argument count\n");
            show_help();
            return 0;
        }
        printf("\nAll servers:\n");
        printf("-----------------------------------------------------\n");
        int res = server_list_all();
        printf("-----------------------------------------------------\n\n");
        if( res == 0 )
            return 1;
		
    } else if( validArg == 0 ){
		
		//Check if user has a valid argument
    	//If not show the help:
        show_help();
		
    }

    return 0;
}


/*
 * =========================================================================== 
 * SQL QUERYS:
 * =========================================================================== 
 */

/*
 *  Open server by server id
 *  Returns 1 on success!
 * ---------------------------------------------------------------------------
 */
int server_open( int *id )
{
    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf(
	"SELECT id, CASE "
	"WHEN protocol='ssh' THEN 'ssh ' || host || ' -p ' || port || ' -l ' || username "
	"WHEN protocol='sftp' THEN 'nautilus sftp://' || username || '@' || host || ':' || port "
	"WHEN protocol='https' THEN 'xdg-open https://' || host || ':' || port "
	"WHEN protocol='http' THEN 'xdg-open http://' || host || ':' || port "
	"END AS execstring "
	"FROM srvl WHERE id='%d' "
	"LIMIT 0,1", *id);
	
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
 *  Add server to database
 *  returns 1 on success
 *  returns 0 on fail
 *  @name Name of the server
 *  @host Ip or domain
 *  @port Server port
 *  @protocol Protocol to be used when opening a server
 * ---------------------------------------------------------------------------
 */
int server_add( char *name, char *host, int *port, 
               char *protocol, char* username )
{
    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf("INSERT INTO srvl('name','host','port',"
                                  "'protocol','username') "
                                  "VALUES('%q', '%q', '%d', '%q', '%q')", 
                                  name, host, *port, protocol, username);
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
 * ---------------------------------------------------------------------------
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
 *  Search database by a string and display result with the callback function
 * ---------------------------------------------------------------------------
 */
int server_search( char *searchstring )
{
    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf("SELECT "
    "id, name, host, port, protocol, username " 
    "FROM srvl WHERE "
    "name LIKE '%%%q%%' OR "
    "host LIKE '%%%q%%' OR "
    "port LIKE '%%%q%%' OR "
    "protocol LIKE '%%%q%%' OR "
    "username LIKE '%%%q%%' "
    "ORDER BY id ASC", searchstring, searchstring, searchstring, 
                                  searchstring, searchstring);

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
 *  List all servers in the database and display with the callback function
 * ---------------------------------------------------------------------------
 */
int server_list_all( )
{
    db_connect();
    char *err_msg = 0;
    char *query = sqlite3_mprintf("SELECT "
    "id, name, host, port, protocol, username FROM srvl ORDER BY id ASC");

    int rc = sqlite3_exec(db, query, server_list_all_callback, 0, &err_msg);
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
 * =========================================================================== 
 * SQL CALLBACKS:
 * =========================================================================== 
 */

/*
 *  Search callback
 *  sqlite calls this function when the search-query is done
 *  NOTE: returns 0 on success!
 * ---------------------------------------------------------------------------
 */
static int server_search_callback(void *NotUsed, int argc, 
                                  char **argv, char **azColName)
{
    printf("%s. %s, %s, %s, %s, %s\n", 
           argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
    return 0;
}

/*
 *  Open callback
 *  sqlite calls this function when the open-query is done
 *  NOTE: returns 0 on success!
 * ---------------------------------------------------------------------------
 */
static int server_open_callback(void *NotUsed, int argc, 
                                char **argv, char **azColName)
{
	printf("Open: %s\n",argv[1]);
    system(argv[1]);
    return 0;
}

/*
 *  List all servers - Callback
 *  sqlite calls this function when the list query is done
 *  NOTE: returns 0 on success!
 * ---------------------------------------------------------------------------
 */
static int server_list_all_callback(void *NotUsed, int argc, 
                                  char **argv, char **azColName)
{
    printf("%s. %s, %s, %s, %s, %s\n", 
           argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
    return 0;
}


/*
 * =========================================================================== 
 * SQL OPEN AND CLOSE:
 * =========================================================================== 
 */

/*
 *  Connect to databasefile:
 *  returns 1 on success
 *  returns 0 on fail
 * ---------------------------------------------------------------------------
 */
int db_connect()
{
    //Connect to datbase and create one if it does not exist:
    int rc = sqlite3_open_v2("srvl.db", &db, 
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    //Create a table if it doesnt exist:
    char *err_msg = 0;
    char *query = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS `srvl` ("
                                "`id`	INTEGER NOT NULL PRIMARY KEY "
                                " AUTOINCREMENT UNIQUE,"
                                "`name`	TEXT,"
                                "`host`	TEXT,"
                                "`port`	INTEGER,"
                                "`protocol`	TEXT,"
                                "`username`	TEXT);");
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
 * ---------------------------------------------------------------------------
 */
int db_close()
{
    if( db != NULL )
        sqlite3_close(db);
}

/*
 * =========================================================================== 
 * OTHER:
 * =========================================================================== 
 */

/*
 *  Ask a question
 *  return 1 on YES
 *  return 0 on NO
 *  *question is the question to be asked
 * ---------------------------------------------------------------------------
 */
int yn_question( char *question )
{
    int result;
	int stdin_garbage;
    char answer;
    while( tolower(answer) != 'y' && tolower(answer) != 'n' )
    {
        printf("%s (y)es or (n)o: ", question);
        answer = getchar();
        if( tolower(answer) == 'y' )
        {
            result = 1;
        } else if( tolower(answer) == 'n' ){
            result = 0;
        }
		//Empty stdin before ending/re-enter the loop:
		while((stdin_garbage=getchar()) != EOF && stdin_garbage != '\n');
    }
    
    return result;
}


/*
 *  Display help:
 * ---------------------------------------------------------------------------
 */
 int show_help()
 {
     printf("\n============================================================\n");
     printf("SERVER HELP\n");
     printf("============================================================\n");
     printf("Add:\t\t server add <name> <host> <port> <protocol> <username>\n");
     printf("------------------------------------------------------------\n");
     printf("Remove:\t\t server remove <id>\n");
     printf("------------------------------------------------------------\n");
     printf("Search:\t\t server search <search string>\n");
     printf("------------------------------------------------------------\n");
     printf("List all:\t server list\n");
     printf("------------------------------------------------------------\n");
     printf("Open by id:\t server o <id>\n");
     printf("------------------------------------------------------------\n");
     printf("Open by name:\t server O <name>\n");
     printf("------------------------------------------------------------\n\n");
 }
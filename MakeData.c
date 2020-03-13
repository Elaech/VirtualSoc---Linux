#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int main(int argc, char* argv[]) {
   sqlite3 *db;
   char *errmsg = 0;
   int rc;
   char *sql;

   rc = sqlite3_open("Databases/Clients.db", &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } 
   rc=sqlite3_exec(db,"BEGIN TRANSACTION;",callback,0,&errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "DROP TABLE IF EXISTS USER;";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "DROP TABLE IF EXISTS POST;";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "DROP TABLE IF EXISTS FRIEND;";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "DROP TABLE IF EXISTS CHAT;";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "DROP TABLE IF EXISTS ROOM;";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "DROP TABLE IF EXISTS REQUEST;";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "CREATE TABLE USER(USERNAME TEXT PRIMARY KEY NOT NULL, PASSWORD TEXT NOT NULL,SALT TEXT NOT NULL, PROFILE INT NOT NULL, IP TEXT NOT NULL, ADMIN_STATUS INT NOT NULL, ONLINE_STATUS INT NOT NULL);";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "CREATE TABLE ROOM(ROOM_NAME TEXT PRIMARY KEY NOT NULL,PASSWORD TEXT, PEOPLE_NUMBER INT NOT NULL, MESSAGE_COUNT INT NOT NULL);";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "CREATE TABLE POST(USERNAME TEXT NOT NULL,POST_TEXT TEXT NOT NULL,VISIBILITY INT NOT NULL);";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "CREATE TABLE CHAT(USERNAME TEXT NOT NULL,ROOM_NAME TEXT NOT NULL,MESAGE TEXT NOT NULL,MESAGE_NUMBER INT NOT NULL);";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "CREATE TABLE FRIEND(USERNAME TEXT NOT NULL,PAL TEXT NOT NULL,FRIEND_TYPE INT NOT NULL,PRIMARY KEY(USERNAME,PAL));";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sql = "CREATE TABLE REQUEST(USERNAME TEXT NOT NULL,PAL TEXT NOT NULL,FRIEND_TYPE INT NOT NULL,PRIMARY KEY(USERNAME,PAL));";
   rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   rc=sqlite3_exec(db,"END TRANSACTION;",callback,0,&errmsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }

   sqlite3_close(db);
   return 0;
}
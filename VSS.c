#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <sqlite3.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <wait.h>
#include <time.h>
#include <fcntl.h>

#define HELPCHAT "[VSS]Chat Actions:\nSending Message: write anything and press enter\nRefreshing Chat Messages: press enter without any text introduced\n/leave - to leave chatroom\nIf nothing new appears after an enter then nothing new has been chatted\n"
#define HELP "[VSS]Command List:\nregister - lets you create an account on VirtualSoc server\nlogin - lets you login into an already created account on VirtualSoc server\nshowpublicusers - shows a list of people that have their profile so that anyone can see their public posts\nshowpublicposts - shows the public posts of a public user of your choosing\nquit - disconnects you from the server\n"
#define HELPLOGGED "[VSS]Command List:\nquit - disconnects and logs out you from the server\nlogout -  logs out you from the server\nfriendlist - displays the list of your friends\naddfriend - sends a friend request to another user\nremovefriend - removes one of your friends\nsetprofile - sets profile to private/public\nmyrequests - displays a list requests sent to you\nremoverequest - cancel one of your/someone elses request\nacceptrequest - lets you accept a pending request from someone\npost - lets you setup and upload a post\neditpost - lets you edit the visibility of a post\nremovepost - lets you delete one of your posts\nshowownposts - displays your posts\nshowposts - show posts of another user\nshowpublicusers - shows a list of all public users\nshowchats - shows all active group chats\ncreatechat - creates an active group chat\njoinchat - join an active group chat\n"
#define VALID_ACC strcpy(sql,"SELECT * FROM USER WHERE USERNAME=\""); \
                strcat(sql,username); \
                strcat(sql,"\";"); \
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0); \
                if( DBresult != SQLITE_OK ){ \
                    fprintf(stderr, "SQL error: %s\n", errmsg); \
                    sqlite3_free(errmsg); \
                    close(client); \
                    sqlite3_close(db); \
                    exit(98); \
                } \
                DBresult=sqlite3_step(DBout); \
                if(DBresult!=SQLITE_ROW){ \
                    strcpy(cmdmsg,"[VSS]Your account has been deleted\n"); \
                    send_data(client,cmdmsg,strlen(cmdmsg)+1); \
                    sqlite3_finalize(DBout); \
                    close(client); \
                    sqlite3_close(db); \
                    exit(37); \
                } \
                sqlite3_finalize(DBout); \

#define FORCELOGOUT strcpy(sql,"UPDATE USER SET ONLINE_STATUS=0 WHERE USERNAME=\""); \
                    strcat(sql,username); \
                    strcat(sql,"\";"); \
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg); \
                    if (DBresult != SQLITE_OK ) { \
                        fprintf(stderr, "SQL error: %s\n", errmsg); \
                        sqlite3_free(errmsg); \
                        close(client); \
                        sqlite3_close(db); \
                        exit(98); \
                    } \
                    logged=0; \
                    admin=0; \
                    close(client); \
                    sqlite3_close(db); \
                    exit(33); \

#define SENDERR sqlite3_close(db); \
                close(client); \
                exit(34); \

#define CHATERR strcpy(sql,"UPDATE USER SET ONLINE_STATUS=0 WHERE USERNAME=\""); \
                        strcat(sql,username); \
                        strcat(sql,"\";"); \
                        DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg); \
                        if (DBresult != SQLITE_OK ) { \
                            fprintf(stderr, "SQL error: %s\n", errmsg); \
                            sqlite3_free(errmsg); \
                            close(client); \
                            sqlite3_close(db); \
                            exit(98); \
                        } \
                        strcpy(sql,"UPDATE ROOM SET PEOPLE_NUMBER=PEOPLE_NUMBER-1 WHERE ROOM_NAME=\""); \
                        strcat(sql,pal); \
                        strcat(sql,"\";"); \
                        DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg); \
                        if (DBresult != SQLITE_OK ) { \
                            fprintf(stderr, "SQL error: %s\n", errmsg); \
                            sqlite3_free(errmsg); \
                            close(client); \
                            sqlite3_close(db); \
                            exit(98); \
                        } \
                        strcpy(sql,"SELECT * FROM ROOM WHERE ROOM_NAME=\""); \
                        strcat(sql,pal); \
                        strcat(sql,"\" AND PEOPLE_NUMBER=0;"); \
                        DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0); \
                        if( DBresult != SQLITE_OK ){ \
                            fprintf(stderr, "SQL error: %s\n", errmsg); \
                            sqlite3_free(errmsg); \
                            close(client); \
                            sqlite3_close(db); \
                            exit(98); \
                        } \
                        DBresult=sqlite3_step(DBout); \
                        if(DBresult==SQLITE_ROW){ \
                            sqlite3_finalize(DBout); \
                            strcpy(sql,"DELETE FROM CHAT WHERE ROOM_NAME=\""); \
                            strcat(sql,pal); \
                            strcat(sql,"\";"); \
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg); \
                            if (DBresult != SQLITE_OK ) { \
                                fprintf(stderr, "SQL error: %s\n", errmsg); \
                                sqlite3_free(errmsg); \
                                close(client); \
                                sqlite3_close(db); \
                                exit(98); \
                            } \
                            strcpy(sql,"DELETE FROM ROOM WHERE PEOPLE_NUMBER=0;"); \
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg); \
                            if (DBresult != SQLITE_OK ) { \
                                fprintf(stderr, "SQL error: %s\n", errmsg); \
                                sqlite3_free(errmsg); \
                                close(client);  \
                                sqlite3_close(db); \
                                exit(98); \
                            } \
                        } \
                        else \
                            sqlite3_finalize(DBout); \
                        logged=0; \
                        admin=0; \
                        close(client); \
                        sqlite3_close(db); \
                        exit(33); \

#define DBERROR fprintf(stderr, "SQL error: %s\n", errmsg); \
                    sqlite3_free(errmsg); \
                    close(client); \
                    sqlite3_close(db); \
                    exit(98); \

#define CHECKFORDB(X) if(checkDBValid(X)==-1){ \
                        strcpy(cmdmsg,"[VSS]Only 0-9, a-z, A-Z and some special characters are available\n"); \
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){ \
                                    SENDERR; \
                        } \
                        continue; \
                    } \

#define CHECKFORDBR(X) if(checkDBValid(X)==-1){ \
                        strcpy(cmdmsg,"[VSS]Only 0-9, a-z, A-Z|\"cancel\" in order to cancel the process\n"); \
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){ \
                                    SENDERR; \
                        } \
                        continue; \
                    } \

#define PORT 5174

extern int errno;


void signal_handler_wr(int signo){
  ;
}
void signal_handler(int signo){
  if(signo==SIGCHLD){
    int status;
    while(waitpid(-1, &status, WNOHANG)>0);
    printf("[VSS]Client disconnected\n");
  }
}
void crypt(char * buff,int key){
    int l=strlen(buff);
    int key1=key;
    for(int i=0;i<l;i++){
        buff[i]=(char)(((int)buff[i]+(key%10))%256+1);
        key=key/10;
        if(key==0)
            key=key1;
    }
}
void decrypt(char * buff,int key){
    int l=strlen(buff);
    int key1=key;
    for(int i=0;i<l;i++){
        buff[i]=(char)(((int)buff[i]-(key%10))%256-1);
        key=key/10;
        if(key==0)
            key=key1;
    }
}
int send_data(int fd,char * buff,int size){
    srand(time(0));
    int key=rand();
    crypt(buff,key);
    key=key+size;
    if(write(fd,&size,sizeof(size))==-1){
        printf("Error in sending message\n");
        return -1;
    }
    if(write(fd,&key,sizeof(key))==-1){
        printf("Error in sending message\n");
        return -1;
    }
    if(write(fd,buff,size)==-1){
        printf("Error in sending message\n");
        return -1;
    }
    return 0;
}


int recv_data(int fd,char * buff){
    int key;
    int size;
    if(read(fd,&size,sizeof(size))<=0){
        perror("Error in receiving message\n");
        return -1;
    }
    if(read(fd,&key,sizeof(key))<=0){
        perror("Error in receiving message\n");
        return -1;
    }
    key=key-size;
    if(read(fd,buff,size)<=0){
        perror("Error in receiving message\n");
        return -1;
    }
    decrypt(buff,key);
    return size;
}
int checkDBValid(const char * s){
    int l=strlen(s);
    for(int i=0;i<l;i++){
        if((s[i]<'0' || s[i]>'9')&&(s[i]<'A' || s[i]>'Z')&&(s[i]<'a' || s[i]>'z')&&s[i]!=' '&&s[i]!='.'&&s[i]!=','&&s[i]!='?'&&s[i]!='!'&&s[i]!=':'&&s[i]!=';'&&s[i]!='-'&&s[i]!='+'){
            return -1;
        }
    }
    return 1;
}
int checkValidUsername(const char * s){
    int l=strlen(s);
    if(strcmp(s,"cancel")==0)
        return 0;
    if(strlen(s)>30 || strlen(s)<8)
        return -1;
    for(int i=0;i<l;i++)
        if((s[i]<'a' || s[i]>'z') && (s[i]<'A' || s[i]>'Z'))
            return -2;
    return 1;
}
int checkValidPassword(const char * s){
    int l=strlen(s);
    if(strcmp(s,"cancel")==0)
        return 0;
    if(strlen(s)>30 || strlen(s)<8)
        return -1;
    int si=0,S=0,n=0;
    for(int i=0;i<l;i++){
        if(s[i]>='a' && s[i]<='z')
            si=1;
        else if(s[i]>='A' && s[i]<='Z')
            S=1;
        else if(s[i]>='0' && s[i]<='9')
            n=1;
    }
    if(si==1 && S==1 && n==1)
        return 1;
    return -2;
}
int checkRowID(const char * s){
    int l=strlen(s);
    if(l==0)
        return -1;
    for(int i=0;i<l;i++)
        if(s[i]<'0' || s[i]>'9')
            return -1;
    return 1;
}
int checkPost(const char * s){
    int l=strlen(s);
    if(l<10 || l>500)
        return -1;
    return 1;
}
char * generateSalt(int pid,long time){
    char * salt=(char*)malloc(1001* sizeof(char));
    int index=0;
    unsigned char c;
    while(pid>0){
        salt[index]= (char)((pid%10)+'0');
        index++;
        pid=pid/10;
    }
    while(time>0){
        salt[index]= (char)((time%10)+'0');
        index++;
        time=time/10;
    }
    
    salt[index]='\0';
    return(salt);
}
char * pass_encrypt(const char * pass, const char * salt){
    char * password=(char*)malloc(sizeof(char)*(strlen(pass)+strlen(salt)+1));
    strcpy(password,pass);
    strcat(password,salt);
    int l=strlen(password);
    for(int i=0;i<l;i++){
        password[i]= (char)((((int)password[i]*(l-i)+i)%10)+'0');
        if(i>0)
            password[i]=(char)(((int)password[i-1]+password[i])%10+'0');
        if(password[i]=='\"' ||password[i]=='\'')
            password[i]=password[i]+1;
    } 
    return(password);
}
int main(){
//Initializing Database
sqlite3 *db;
sqlite3_stmt *DBout;
char *errmsg = 0;
int DBresult;
int queryInt;
char sql[1024];
DBresult = sqlite3_open("Databases/Clients.db", &db);
if( DBresult ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
} 
//Creating Socket
int sd;
if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
    perror ("[VSS]Error in creating socket\n");
    return errno;
}
//TCP keepalive
int optval=1;
socklen_t optlen=sizeof(optval);
if(getsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
      perror("getsockopt()");
      close(sd);
      exit(EXIT_FAILURE);
}
//server structure
struct sockaddr_in server;	
struct sockaddr_in from;	
bzero (&server, sizeof (server));
bzero (&from, sizeof (from));


server.sin_family = AF_INET;	
server.sin_addr.s_addr = htonl (INADDR_ANY);
server.sin_port = htons (PORT);

//bind
if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1){
    perror ("[VSS]Eroare in bind\n");
    return errno;
}

//listen
if (listen (sd, 10) == -1){
    perror ("[VSS]Error in listen\n");
    return errno;
}

//Variables for communication////////////////////////////////////////////////////
char cmdmsg[128];		//command buffer
char post_list[16384];
char chat_message[300];
char post[600];
char * clientIP; 
char *salt, *pass;
int valid;
int proceed=0;
char username[35],password[35],pal[35];
int logged=0;
int counter;
int admin=0;
/////////////////////////////////////////////////////////////////////////////////
while(1){
    int svChild,client;
    int addLen=sizeof(from);

    printf("[VSS]Awaiting connections...\n");
    //accept
    client=accept(sd,(struct sockaddr *) &from, &addLen);
    if (client < 0){
	    perror ("[VSS]Error in accepting client\n");
	    continue;
	}

    //creating svChild
    while((svChild=fork())==-1){
        perror("[VSS]Error in detaching child; Trying again...\n");
    }
    //svChild
    if(svChild==0){
        clientIP=inet_ntoa(from.sin_addr);
        while(1){
            fflush(stdout);
            if(recv_data(client,cmdmsg)==-1){
                if(logged==1){
                    strcpy(sql,"UPDATE USER SET ONLINE_STATUS=0 WHERE USERNAME=\"");
                    strcat(sql,username);
                    strcat(sql,"\";");
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                    if (DBresult != SQLITE_OK ){
                        DBERROR;
                    }
                    logged=0;
                    admin=0;
                    close(client);
                    sqlite3_close(db);
                    exit(33);
                }
                close(client);
                sqlite3_close(db);
                exit(33);
            }
            //QUIT
            if(strcmp(cmdmsg,"quit")==0 && logged==0){
                send_data(client,cmdmsg,strlen(cmdmsg)+1);
                sqlite3_close(db);
                close(client);
                exit(1);
            }
            //HELP NOT LOGGED
            else if(strcmp(cmdmsg,"help")==0 && logged==0){
                strcpy(post,HELP);
                if(send_data(client,post,strlen(post)+1)==-1){
                    SENDERR;
                }
            }
            //REGISTER
            else if(strcmp(cmdmsg,"register")==0 && logged==0){
                //CHECK SPACE
                strcpy(sql,"SELECT COUNT(*) FROM USER");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                DBresult=sqlite3_step(DBout);
                if(DBresult==SQLITE_ROW)
                    if(sqlite3_column_int(DBout,0)>=200){
                        strcpy(cmdmsg,"[VSS]Maximum number of accounts on VirtualSoc reached\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            SENDERR;
                        }
                        continue;
                    }
                sqlite3_finalize(DBout);
                //CHECK IP
                proceed=0;
                strcpy(sql,"SELECT COUNT(*) FROM USER WHERE IP=\"");
                strcat(sql,clientIP);
                strcat(sql,"\" AND ADMIN_STATUS=0;");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult==SQLITE_ROW){ 
                    queryInt=sqlite3_column_int(DBout,0);
                    if(queryInt<11)
                        proceed=1;
                }
                else
                    proceed=1;
                sqlite3_finalize(DBout);
                if(!proceed){
                    strcpy(cmdmsg,"[VSS]There are already 3 accounts created on this IP\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        SENDERR;
                    }
                }
                else{
                    //VERIFY USERNAME
                    strcpy(cmdmsg,"[VSS]Enter desired username (8-30 EN letters) | Type \"cancel\" in order to cancel the proccess\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        SENDERR;
                    }
                    while(1){
                        if(recv_data(client,cmdmsg)==-1){
                            close(client);
                            sqlite3_close(db);
                            exit(33);
                        }
                        valid=checkValidUsername(cmdmsg);
                        if(valid == -1)
                          strcpy(cmdmsg,"[VSS]Size is wrong (8-30)| Try again or type \"cancel\"\n");
                        else if(valid == -2)
                            strcpy(cmdmsg,"[VSS]Contains illegal characters| Try again or type \"cancel\"\n");
                        else if(valid== 0){
                            strcpy(cmdmsg,"[VSS]Successfuly cancelled!\n");
                            if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                SENDERR;
                            }
                            break;
                        }
                        else if(valid==1){
                            //CHECK USERNAME
                            strcpy(sql,"SELECT * FROM USER WHERE USERNAME=\"");
                            strcat(sql,cmdmsg);
                            strcat(sql,"\";");
                            proceed=0;
                            DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                            if( DBresult != SQLITE_OK ){
                                DBERROR;
                            }
                            DBresult=sqlite3_step(DBout);
                            if(DBresult==SQLITE_ROW)
                                proceed=0;
                            else
                                proceed=1;
                            sqlite3_finalize(DBout);
                            if(proceed){
                                //VERIFY PASSWORD
                                strcpy(username,cmdmsg);
                                strcpy(cmdmsg,"[VSS]Enter desired password (8-30 must have small and big letters and numbers) |\"cancel\" to cancel the proccess\n");
                                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                    SENDERR;
                                }
                                while(1){
                                    if(recv_data(client,cmdmsg)==-1){
                                        close(client);
                                        sqlite3_close(db);
                                        exit(33);
                                    }
                                    valid=checkValidPassword(cmdmsg);
                                    if(valid==1){
                                        CHECKFORDBR(cmdmsg);
                                        strcpy(sql,"INSERT INTO USER VALUES(\"");
                                        strcat(sql,username);
                                        strcat(sql,"\",\"");
                                        salt=generateSalt((int)getpid(),(long)time(0));
                                        pass=pass_encrypt(cmdmsg,salt);
                                        strcat(sql,pass);
                                        strcat(sql,"\",\"");
                                        strcat(sql,salt);
                                        free(salt);
                                        free(pass);
                                        strcat(sql,"\",0,\"");
                                        strcat(sql,clientIP);
                                        strcat(sql,"\",0,0);");
                                        DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                                        if (DBresult != SQLITE_OK ) {
                                            DBERROR;
                                        }
                                        strcpy(cmdmsg,"[VSS]Account successfuly created!Try to log in\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                        break;
                                    }
                                    else if(valid==0){
                                        strcpy(cmdmsg,"[VSS]Successfuly cancelled!\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                        break;
                                    }
                                    else if(valid==-1){
                                        strcpy(cmdmsg,"[VSS]Password must be 8 to 30 characters long |\"cancel\" to cancel the proccess\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                    }
                                    else if(valid==-2){
                                        strcpy(cmdmsg,"[VSS]Password must have only small and big letters and numbers |\"cancel\" to cancel the proccess\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                    }
                                }
                                break;
                            }
                            else{
                                strcpy(cmdmsg,"[VSS]Username already exists!Try again or type \"cancel\"\n");
                                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                    SENDERR;
                                }
                                continue;
                            }
                        }

                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            SENDERR;
                        }
                    }
                }
            }
            else if(strcmp(cmdmsg,"register40312")==0 && logged==0){
                //CHECK SPACE
                strcpy(sql,"SELECT COUNT(*) FROM USER");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                DBresult=sqlite3_step(DBout);
                if(DBresult==SQLITE_ROW)
                    if(sqlite3_column_int(DBout,0)>=200){
                        strcpy(cmdmsg,"[VSS]Maximum number of accounts on VirtualSoc reached\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            SENDERR;
                        }
                        continue;
                    }
                sqlite3_finalize(DBout);
                //CHECK IP
                proceed=0;
                strcpy(sql,"SELECT * FROM USER WHERE IP=\"");
                strcat(sql,clientIP);
                strcat(sql,"\" AND ADMIN_STATUS=1;");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult==SQLITE_ROW){ 
                    proceed=0;
                }
                else
                    proceed=1;
                sqlite3_finalize(DBout);
                if(!proceed){
                    strcpy(cmdmsg,"[VSS]There is already 1 admin account created on this IP\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        SENDERR;
                    }
                }
                else{
                    //VERIFY USERNAME
                    strcpy(cmdmsg,"[VSS]Enter desired username (8-30 EN letters) | Type \"cancel\" in order to cancel the proccess\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        SENDERR;
                    }
                    while(1){
                        if(recv_data(client,cmdmsg)==-1){
                            close(client);
                            sqlite3_close(db);
                            exit(33);
                        }
                        valid=checkValidUsername(cmdmsg);
                        if(valid == -1)
                          strcpy(cmdmsg,"[VSS]Size is wrong (8-30)| Try again or type \"cancel\"\n");
                        else if(valid == -2)
                            strcpy(cmdmsg,"[VSS]Contains illegal characters| Try again or type \"cancel\"\n");
                        else if(valid== 0){
                            strcpy(cmdmsg,"[VSS]Successfuly cancelled!\n");
                            if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                SENDERR;
                            }
                            break;
                        }
                        else if(valid==1){
                            //CHECK USERNAME
                            strcpy(sql,"SELECT * FROM USER WHERE USERNAME=\"");
                            strcat(sql,cmdmsg);
                            strcat(sql,"\";");
                            proceed=0;
                            DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                            if( DBresult != SQLITE_OK ){
                                DBERROR;
                            }
                            DBresult=sqlite3_step(DBout);
                            if(DBresult==SQLITE_ROW)
                                proceed=0;
                            else
                                proceed=1;
                            sqlite3_finalize(DBout);
                            if(proceed){
                                //VERIFY PASSWORD
                                strcpy(username,cmdmsg);
                                strcpy(cmdmsg,"[VSS]Enter desired password (8-30 must have small and big letters and numbers) |\"cancel\" to cancel the proccess\n");
                                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                    SENDERR;
                                }
                                while(1){
                                    if(recv_data(client,cmdmsg)==-1){
                                        close(client);
                                        sqlite3_close(db);
                                        exit(33);
                                    }
                                    valid=checkValidPassword(cmdmsg);
                                    if(valid==1){
                                        CHECKFORDBR(cmdmsg);
                                        strcpy(sql,"INSERT INTO USER VALUES(\"");
                                        strcat(sql,username);
                                        strcat(sql,"\",\"");
                                        salt=generateSalt((int)getpid(),(long)time(0));
                                        pass=pass_encrypt(cmdmsg,salt);
                                        strcat(sql,pass);
                                        strcat(sql,"\",\"");
                                        strcat(sql,salt);
                                        free(salt);
                                        free(pass);
                                        strcat(sql,"\",0,\"");
                                        strcat(sql,clientIP);
                                        strcat(sql,"\",1,0);");
                                        DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                                        if (DBresult != SQLITE_OK ) {
                                            DBERROR;
                                        }
                                        strcpy(cmdmsg,"[VSS]Account successfuly created!Try to log in\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                        break;
                                    }
                                    else if(valid==0){
                                        strcpy(cmdmsg,"[VSS]Successfuly cancelled!\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                        break;
                                    }
                                    else if(valid==-1){
                                        strcpy(cmdmsg,"[VSS]Password must be 8 to 30 characters long |\"cancel\" to cancel the proccess\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                    }
                                    else if(valid==-2){
                                        strcpy(cmdmsg,"[VSS]Password must have must have small and big letters and numbers |\"cancel\" to cancel the proccess\n");
                                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                            SENDERR;
                                        }
                                    }
                                }
                                break;
                            }
                            else{
                                strcpy(cmdmsg,"[VSS]Username already exists!Try again or type \"cancel\"\n");
                                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                    SENDERR;
                                }
                                continue;
                            }
                        }
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            SENDERR;
                        }
                    }
                }
            }
            //SHOW LIST OF PUBLIC USERS
            else if(strcmp(cmdmsg,"showpublicusers")==0 && logged==0){
                strcpy(sql,"SELECT * FROM USER WHERE PROFILE=0;");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                post_list[0]='\0';
                while(sqlite3_step(DBout)==SQLITE_ROW){
                    strcat(post_list,"[VSS]");
                    strcat(post_list,sqlite3_column_text(DBout,0));
                    strcat(post_list,"\n");
                }
                sqlite3_finalize(DBout);
                if(strlen(post_list)==0)
                    strcat(post_list,"[VSS]No public users found\n");
                else
                    post_list[strlen(post_list)]='\0';
                if(send_data(client,post_list,strlen(post_list)+1)==-1){
                    SENDERR;
                }                
            }
            //SHOW PUBLIC POSTS OF A PUBLIC USER
            else if(strcmp(cmdmsg,"showpublicposts")==0 && logged==0){
                //CHECK USER EXISTANCE
                strcpy(cmdmsg,"[VSS]Name a User to show public posts for\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    SENDERR;
                }
                if(recv_data(client,cmdmsg)==-1){
                        close(client);
                        sqlite3_close(db);
                        exit(33);
                }
                CHECKFORDB(cmdmsg);
                strcpy(sql,"SELECT * FROM USER WHERE PROFILE=0 AND USERNAME=\"");
                strcat(sql,cmdmsg);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                if(sqlite3_step(DBout)!=SQLITE_ROW){
                    strcpy(post_list,"[VSS]Username with public profile specified does not exist\n");
                    if(send_data(client,post_list,strlen(post_list)+1)==-1){
                        SENDERR;
                    }
                    sqlite3_finalize(DBout);
                }
                else{
                    //SELECT POST IF EXISTS
                    sqlite3_finalize(DBout);
                    strcpy(sql,"SELECT POST_TEXT FROM POST WHERE VISIBILITY=0 AND USERNAME=\"");
                    strcat(sql,cmdmsg);
                    strcat(sql,"\";");
                    DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                    if( DBresult != SQLITE_OK ){
                        DBERROR;
                    }
                    post_list[0]='\0';
                    while(sqlite3_step(DBout)==SQLITE_ROW){
                        strcat(post_list,"[VSS]");
                        strcat(post_list,sqlite3_column_text(DBout,0));
                        strcat(post_list,"\n\n");
                    }
                    sqlite3_finalize(DBout);
                    if(strlen(post_list)==0)
                        strcpy(post_list,"[VSS]No public posts for this user have been found\n");
                    else
                        post_list[strlen(post_list)]='\0';
                    if(send_data(client,post_list,strlen(post_list)+1)==-1){
                        SENDERR;
                    }
                }
            }
            //LOGIN
            else if(strcmp(cmdmsg,"login")==0 && logged==0){
                strcpy(cmdmsg,"[VSS]Username:\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    SENDERR;
                }
                if(recv_data(client,username)==-1){
                        close(client);
                        sqlite3_close(db);
                        exit(33);
                }
                CHECKFORDB(username);
                strcpy(cmdmsg,"[VSS]Password:\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    SENDERR;
                }
                if(recv_data(client,password)==-1){
                        close(client);
                        sqlite3_close(db);
                        exit(33);
                }
                CHECKFORDB(password);
                //VERIFY ACCOUNTS EXISTANCE
                strcpy(sql,"SELECT ONLINE_STATUS,ADMIN_STATUS,SALT,PASSWORD FROM USER WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]Username or password are incorrect\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        SENDERR;
                    }
                }
                else{
                    //CHECK PASSWORD
                    queryInt=sqlite3_column_int(DBout,0);
                    valid=sqlite3_column_int(DBout,1);
                    strcpy(cmdmsg,sqlite3_column_text(DBout,2));
                    strcpy(post,sqlite3_column_text(DBout,3));
                    sqlite3_finalize(DBout);
                    pass=pass_encrypt(password,cmdmsg);
                    if(strcmp(pass,post)!=0){
                        strcpy(cmdmsg,"[VSS]Username or password are incorrect\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        SENDERR;
                        }
                        free(pass);
                        continue;
                    }
                    //VERIFY ACCOUNTS ONLINE STATUS
                    if(queryInt==1){
                        strcpy(cmdmsg,"[VSS]Account already logged in\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            SENDERR;
                        }
                    }
                    else{
                        //SETTING ONLINE STATUS AND ACCEPTING
                        strcpy(sql,"UPDATE USER SET ONLINE_STATUS=1 WHERE USERNAME=\"");
                        strcat(sql,username);
                        strcat(sql,"\";");
                        DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                        if (DBresult != SQLITE_OK ) {
                            DBERROR;
                        }
                        logged=1;
                        admin=valid;
                        strcpy(cmdmsg,"[VSS]Successfuly logged into your account\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            SENDERR;
                        }
                    }
                }
            }
            //LOGOUT
            else if(strcmp(cmdmsg,"logout")==0 && logged==1){
                //UPDATE ONLINE STATUS AND ACCEPTING
                strcpy(sql,"UPDATE USER SET ONLINE_STATUS=0 WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                logged=0;
                admin=0;
                strcpy(cmdmsg,"[VSS]Successfuly logged out of your account\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    SENDERR;
                }
            }
            //HELP LOGGED
            else if(strcmp(cmdmsg,"help")==0 && logged==1){
                VALID_ACC;
                strcpy(post,HELPLOGGED);
                if(send_data(client,post,strlen(post)+1)==-1){
                    FORCELOGOUT;
                }
            }
            //LOGOUT AND QUIT
            else if(strcmp(cmdmsg,"quit")==0 && logged==1){
                VALID_ACC;
                strcpy(sql,"UPDATE USER SET ONLINE_STATUS=0 WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                logged=0;
                admin=0;
                strcpy(cmdmsg,"quit");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    SENDERR;
                }
                sqlite3_close(db);
                close(client);
                exit(1);
            }
            //SHOW FRIEND LIST
            else if(strcmp(cmdmsg,"friendlist")==0 && logged==1){
                VALID_ACC;
                strcpy(sql,"SELECT PAL,FRIEND_TYPE FROM FRIEND WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                post_list[0]='\0';
                while(sqlite3_step(DBout)==SQLITE_ROW){
                    strcat(post_list,"[VSS]");
                    strcat(post_list,sqlite3_column_text(DBout,0));
                    if(sqlite3_column_int(DBout,1)==1)
                        strcat(post_list," ### friend");
                    else
                        strcat(post_list," ### closefriend");
                    strcat(post_list,"\n");
                }
                sqlite3_finalize(DBout);
                if(strlen(post_list)==0)
                        strcpy(post_list,"[VSS]No friends in your friendlist\n");
                if(send_data(client,post_list,strlen(post_list)+1)==-1){
                    FORCELOGOUT;
                }
            }
            //ADD FRIEND
            else if(strcmp(cmdmsg,"addfriend")==0 && logged==1){
                VALID_ACC;
                strcpy(sql,"SELECT COUNT(*) FROM FRIEND WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                proceed=0;
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                if(DBresult==SQLITE_ROW){ 
                    queryInt=sqlite3_column_int(DBout,0);
                    if(queryInt<100)
                        proceed=1;
                }
                else
                    proceed=1;
                sqlite3_finalize(DBout);
                if(proceed==0){
                    strcpy(cmdmsg,"[VSS]You have the maximum amount of friends\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(cmdmsg,"[VSS]What is the username?\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(pal);
                if(strcmp(pal,username)==0){
                    strcpy(cmdmsg,"[VSS]You cannot create relations with yourself\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                //CHECK EXISTANCE
                strcpy(sql,"SELECT * FROM USER WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                if(sqlite3_step(DBout)!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No account with such username exists\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                }
                else{
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]Add as friend or closefriend?\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    if(recv_data(client,cmdmsg)==-1){
                        FORCELOGOUT;
                    }
                    proceed=0;
                    if(strcmp(cmdmsg,"friend")==0)
                        valid=1;
                    else if(strcmp(cmdmsg,"closefriend")==0)
                        valid=2;
                    else{
                        strcpy(cmdmsg,"[VSS]No such relation exists\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            FORCELOGOUT;
                        }
                        continue;
                    }
                    //CHECK IF RELATION EXISTS
                    strcpy(sql,"SELECT FRIEND_TYPE FROM FRIEND WHERE USERNAME=\"");
                    strcat(sql,username);
                    strcat(sql,"\" AND PAL=\"");
                    strcat(sql,pal);
                    strcat(sql,"\";");
                    DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                    if( DBresult != SQLITE_OK ){
                        DBERROR;
                    }
                    if(sqlite3_step(DBout)==SQLITE_ROW){
                        if(valid!=sqlite3_column_int(DBout,0))
                            proceed=1;
                        else{
                            sqlite3_finalize(DBout);
                            strcpy(cmdmsg,"[VSS]You are already in that type of relation with your pal\n");
                            if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                FORCELOGOUT;
                            }
                            continue;
                        }
                    }
                    else
                        proceed=1;
                    sqlite3_finalize(DBout);
                    if(proceed==1){
                        //CHECK IF REQUEST ALREADY EXISTS
                        strcpy(sql,"SELECT * FROM REQUEST WHERE USERNAME=\"");
                        strcat(sql,username);
                        strcat(sql,"\" AND PAL=\"");
                        strcat(sql,pal);
                        strcat(sql,"\";");
                        DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                        if( DBresult != SQLITE_OK ){
                            DBERROR;
                        }
                        if(sqlite3_step(DBout)==SQLITE_ROW){
                            sqlite3_finalize(DBout);
                            strcpy(cmdmsg,"[VSS]You have already a request pending to that user\n");
                            if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                FORCELOGOUT;
                            }
                        }
                        else{
                            sqlite3_finalize(DBout);
                            strcpy(sql,"SELECT * FROM REQUEST WHERE USERNAME=\"");
                            strcat(sql,pal);
                            strcat(sql,"\" AND PAL=\"");
                            strcat(sql,username);
                            strcat(sql,"\";");
                            DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                            if( DBresult != SQLITE_OK ){
                                DBERROR;
                            }
                            if(sqlite3_step(DBout)==SQLITE_ROW){
                                sqlite3_finalize(DBout);
                                strcpy(cmdmsg,"[VSS]You have already a request pending from that user\n");
                                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                    FORCELOGOUT;
                                }
                            }
                            else{
                                sqlite3_finalize(DBout);
                                VALID_ACC;
                                strcpy(sql,"INSERT INTO REQUEST VALUES(\"");
                                strcat(sql,username);
                                strcat(sql,"\",\"");
                                strcat(sql,pal);
                                strcat(sql,"\",");
                                sprintf(post,"%d",valid);
                                strcat(sql,post);
                                strcat(sql,");");
                                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                                if (DBresult != SQLITE_OK ) {
                                    DBERROR;
                                }
                                strcpy(cmdmsg,"[VSS]Request has been sent\n");
                                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                                    FORCELOGOUT;
                                }
                            }
                        }
                    }
                }
            }
            //REMOVE FRIEND
            else if(strcmp(cmdmsg,"removefriend")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Which friend do you want to remove?\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT
                }
                CHECKFORDB(pal);
                //CHECK VALID FRIEND
                strcpy(sql,"SELECT * FROM FRIEND WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                if(sqlite3_step(DBout)!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No such friend in your friendlist\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                sqlite3_finalize(DBout);
                strcpy(sql,"DELETE FROM FRIEND WHERE (USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,pal);
                strcat(sql,"\") OR (USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,username);
                strcat(sql,"\");");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Friend removed\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }

            }
            else if(strcmp(cmdmsg,"setprofile")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Set your profile public or private?\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,cmdmsg)==-1){
                    FORCELOGOUT;
                }
                if(strcmp(cmdmsg,"public")==0){
                    proceed=0;
                    sprintf(cmdmsg,"%d",proceed);
                }
                else if(strcmp(cmdmsg,"private")==0){
                    proceed=1;
                    sprintf(cmdmsg,"%d",proceed);
                }
                else{
                    strcpy(cmdmsg,"[VSS]No such status for the profile exists\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                }
                strcpy(sql,"SELECT PROFILE FROM USER WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    strcpy(cmdmsg,"[VSS]Your account has been deleted\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    sqlite3_finalize(DBout);
                    close(client);
                    sqlite3_close(db);
                    exit(37);
                }
                valid=sqlite3_column_int(DBout,0);
                sqlite3_finalize(DBout);
                if(valid==proceed){
                    strcpy(cmdmsg,"[VSS]Profile already set this way\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                }
                else{
                    strcpy(sql,"UPDATE USER SET PROFILE=");
                    strcat(sql,cmdmsg);
                    strcat(sql," WHERE USERNAME=\"");
                    strcat(sql,username);
                    strcat(sql,"\";");
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                    if (DBresult != SQLITE_OK ) {
                        DBERROR;
                    }
                    strcpy(cmdmsg,"[VSS]Profile has been set\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                }
            }
            else if(strcmp(cmdmsg,"myrequests")==0 && logged==1){
                VALID_ACC; 
                strcpy(sql,"SELECT PAL,FRIEND_TYPE FROM REQUEST WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                strcpy(post_list,"[VSS]Requests sent by you:\n");
                while(sqlite3_step(DBout)==SQLITE_ROW){
                    strcat(post_list,"[VSS]");
                    strcat(post_list,sqlite3_column_text(DBout,0));
                    valid=sqlite3_column_int(DBout,1);
                    strcat(post_list, " ### ");
                    if(valid==1)
                        strcat(post_list,"friend\n");
                    else
                        strcat(post_list,"closefriend\n");
                }
                sqlite3_finalize(DBout);
                if(strcmp(post_list,"[VSS]Requests sent by you:\n")==0)
                    strcpy(post_list,"[VSS]You have no requests sent\n");
                strcat(post_list,"\n");
                strcpy(sql,"SELECT USERNAME,FRIEND_TYPE FROM REQUEST WHERE PAL=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    strcat(post_list,"[VSS]You have no requests received\n");
                }
                else{
                    strcat(post_list,"[VSS]Requests received by you:\n");
                    while(DBresult==SQLITE_ROW){
                        strcat(post_list,"[VSS]");
                        strcat(post_list,sqlite3_column_text(DBout,0));
                        valid=sqlite3_column_int(DBout,1);
                        strcat(post_list, " ### ");
                        if(valid==1)
                            strcat(post_list,"friend\n");
                        else
                            strcat(post_list,"closefriend\n");
                        DBresult=sqlite3_step(DBout);
                    }
                }
                sqlite3_finalize(DBout);
                if(send_data(client,post_list,strlen(post_list)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"removerequest")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Enter the name to remove request from\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(pal);
                //CHECK VALID REQUEST
                strcpy(sql,"SELECT * FROM REQUEST WHERE (USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,pal);
                strcat(sql,"\") OR (USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,username);
                strcat(sql,"\");");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                if(sqlite3_step(DBout)!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No such request in your requests list\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                sqlite3_finalize(DBout);
                strcpy(sql,"DELETE FROM REQUEST WHERE (USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,pal);
                strcat(sql,"\") OR (USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,username);
                strcat(sql,"\");");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Request has been removed\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }

            }
            else if(strcmp(cmdmsg,"acceptrequest")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Enter the name to accept request from\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(pal);
                strcpy(sql,"SELECT USERNAME,FRIEND_TYPE FROM REQUEST WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    strcpy(cmdmsg,"[VSS]No such request exists in your requestlist\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    sqlite3_finalize(DBout);
                }
                else{
                    valid=sqlite3_column_int(DBout,1);
                    sqlite3_finalize(DBout);
                    VALID_ACC;
                    strcpy(sql,"DELETE FROM FRIEND WHERE (USERNAME=\"");
                    strcat(sql,username);
                    strcat(sql,"\" AND PAL=\"");
                    strcat(sql,pal);
                    strcat(sql,"\") OR (USERNAME=\"");
                    strcat(sql,pal);
                    strcat(sql,"\" AND PAL=\"");
                    strcat(sql,username);
                    strcat(sql,"\");");
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                    if (DBresult != SQLITE_OK ) {
                        DBERROR;
                    }
                    strcpy(sql,"INSERT INTO FRIEND VALUES(\"");
                    strcat(sql,username);
                    strcat(sql,"\",\"");
                    strcat(sql,pal);
                    strcat(sql,"\",");
                    sprintf(cmdmsg,"%d",valid);
                    strcat(sql,cmdmsg);
                    strcat(sql,");");
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                    if (DBresult != SQLITE_OK ) {
                        DBERROR;
                    }
                    strcpy(sql,"INSERT INTO FRIEND VALUES(\"");
                    strcat(sql,pal);
                    strcat(sql,"\",\"");
                    strcat(sql,username);
                    strcat(sql,"\",");
                    strcat(sql,cmdmsg);
                    strcat(sql,");");
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                    if (DBresult != SQLITE_OK ) {
                        DBERROR;
                    }
                    strcpy(sql,"DELETE FROM REQUEST WHERE (USERNAME=\"");
                    strcat(sql,username);
                    strcat(sql,"\" AND PAL=\"");
                    strcat(sql,pal);
                    strcat(sql,"\") OR (USERNAME=\"");
                    strcat(sql,pal);
                    strcat(sql,"\" AND PAL=\"");
                    strcat(sql,username);
                    strcat(sql,"\");");
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                    if (DBresult != SQLITE_OK ) {
                        DBERROR;
                    }
                    strcpy(cmdmsg,"[VSS]Request accepted\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                }
            }
            else if(strcmp(cmdmsg,"post")==0 && logged==1){ 
                VALID_ACC;
                strcpy(sql,"SELECT COUNT(*) FROM POST WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                valid=sqlite3_column_int(DBout,0);
                sqlite3_finalize(DBout);
                if(valid>=10){
                    strcpy(cmdmsg,"[VSS]You already have the maximum of 10 posts\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(cmdmsg,"[VSS]What message do you wanna post? (10-500 EN chars)\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,post_list)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(post_list);
                if(checkPost(post_list)==-1){
                    strcpy(cmdmsg,"[VSS]Message requirements not met\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                }
                else{
                    strcpy(cmdmsg,"[VSS]Visibility set to public, friends or closefriends?\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    if(recv_data(client,cmdmsg)==-1){
                        FORCELOGOUT;
                    }
                    if(strcmp(cmdmsg,"public")==0)
                        valid=0;
                    else if(strcmp(cmdmsg,"friends")==0)
                        valid=1;
                    else if(strcmp(cmdmsg,"closefriends")==0)
                        valid=2;
                    else{
                        strcpy(cmdmsg,"[VSS]No such visibility option\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            FORCELOGOUT;
                        }
                        continue;
                    }
                    sprintf(post,"%d",valid);
                    VALID_ACC;
                    strcpy(sql,"INSERT INTO POST VALUES(\"");
                    strcat(sql,username);
                    strcat(sql,"\",\"");
                    strcat(sql,post_list);
                    strcat(sql,"\",");
                    strcat(sql,post);
                    strcat(sql,");");
                    DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                    if (DBresult != SQLITE_OK ) {
                        DBERROR;
                    }
                    strcpy(cmdmsg,"[VSS]Message has been posted\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                }
            }
            else if(strcmp(cmdmsg,"editpost")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]What id does your post have?\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,cmdmsg)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(cmdmsg);
                if(checkRowID(cmdmsg)==-1){
                    strcpy(cmdmsg,"[VSS]Id format unkown\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(sql,"SELECT USERNAME,VISIBILITY FROM POST WHERE ROWID=");
                strcat(sql,cmdmsg);
                strcat(sql,";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No such post id found\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(post,sqlite3_column_text(DBout,0));
                if(strcmp(post,username)!=0){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]That post is not yours\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                proceed=sqlite3_column_int(DBout,1);
                sqlite3_finalize(DBout);
                strcpy(post,cmdmsg);
                strcpy(cmdmsg,"[VSS]Set post to public, friends or closefriends?\n");
                VALID_ACC;
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,cmdmsg)==-1){
                    FORCELOGOUT;
                }
                if(strcmp(cmdmsg,"public")==0)
                    valid=0;
                else if(strcmp(cmdmsg,"friends")==0)
                    valid=1;
                else if(strcmp(cmdmsg,"closefriends")==0)
                    valid=2;
                else{
                    strcpy(cmdmsg,"[VSS]No such visibility option\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                if(proceed==valid){
                    strcpy(cmdmsg,"[VSS]Visibility already set this way\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue; 
                }
                sprintf(cmdmsg,"%d",valid);
                VALID_ACC;
                strcpy(sql,"UPDATE POST SET VISIBILITY=");
                strcat(sql,cmdmsg);
                strcat(sql," WHERE ROWID=");
                strcat(sql,post);
                strcat(sql,";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Post visibility edited\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"removepost")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]What id does your post have?\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,cmdmsg)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(cmdmsg);
                if(checkRowID(cmdmsg)==-1){
                    strcpy(cmdmsg,"[VSS]Id format unkown\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(sql,"SELECT USERNAME FROM POST WHERE ROWID=");
                strcat(sql,cmdmsg);
                strcat(sql,";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No such post id found\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(post,sqlite3_column_text(DBout,0));
                if(strcmp(post,username)!=0){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]That post is not yours\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                sqlite3_finalize(DBout);
                VALID_ACC;
                strcpy(sql,"DELETE FROM POST WHERE ROWID=");
                strcat(sql,cmdmsg);
                strcat(sql,";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Post deleted\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"showownposts")==0 && logged==1){
                VALID_ACC;
                strcpy(sql,"SELECT POST_TEXT,VISIBILITY,ROWID FROM POST WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                post_list[0]='\0';
                while(sqlite3_step(DBout)==SQLITE_ROW){
                    strcat(post_list,"[VSS]PostID: ");
                    sprintf(post,"%d",sqlite3_column_int(DBout,2));
                    strcat(post_list,post);
                    strcat(post_list," ### Visibility: ");
                    valid=sqlite3_column_int(DBout,1);
                    if(valid==0)
                        strcat(post_list,"public\n");
                    else if(valid==1)
                        strcat(post_list,"friends\n");
                    else
                        strcat(post_list,"closefriends\n");
                    strcat(post_list,sqlite3_column_text(DBout,0));
                    strcat(post_list,"\n");
                }
                sqlite3_finalize(DBout);
                if(strlen(post_list)==0)
                        strcpy(post_list,"[VSS]You dont have any posts\n");
                if(send_data(client,post_list,strlen(post_list)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"showposts")==0 && logged==1){ 
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Insert username to show posts from\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(pal);
                strcpy(sql,"SELECT FRIEND_TYPE FROM FRIEND WHERE USERNAME=\"");
                strcat(sql,username);
                strcat(sql,"\" AND PAL=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW)
                    valid=0;
                else
                    valid=sqlite3_column_int(DBout,0);
                sqlite3_finalize(DBout);
                post_list[0]='\0';
                strcpy(sql,"SELECT POST_TEXT,VISIBILITY,ROWID FROM POST WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                while(sqlite3_step(DBout)==SQLITE_ROW){
                    proceed=sqlite3_column_int(DBout,1);
                    if(proceed<=valid){
                        strcat(post_list,"[VSS]PostID: ");
                        sprintf(post,"%d",sqlite3_column_int(DBout,2));
                        strcat(post_list,post);
                        strcat(post_list," ### Visibility: ");
                        if(proceed==0)
                            strcat(post_list,"public");
                        else if(proceed==1)
                            strcat(post_list,"friends");
                        else
                            strcat(post_list,"closefriends");
                        strcat(post_list,"\n");
                        strcat(post_list,sqlite3_column_text(DBout,0));
                        strcat(post_list,"\n\n");
                    }
                }
                sqlite3_finalize(DBout);
                if(strlen(post_list)==0)
                    strcpy(post_list,"[VSS]No post found to be shown\n");
                if(send_data(client,post_list,strlen(post_list)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"showpublicusers")==0 && logged==1){
                VALID_ACC;
                strcpy(sql,"SELECT * FROM USER WHERE PROFILE=0;");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                post_list[0]='\0';
                while(sqlite3_step(DBout)==SQLITE_ROW ){
                    strcat(post_list,"[VSS]");
                    strcat(post_list,sqlite3_column_text(DBout,0));
                    strcat(post_list,"\n");
                }
                sqlite3_finalize(DBout);
                if(strlen(post_list)==0)
                    strcat(post_list,"[VSS]No public users found\n");
                else
                    post_list[strlen(post_list)]='\0';
                if(send_data(client,post_list,strlen(post_list)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"showchats")==0 && logged==1){
                VALID_ACC;
                strcpy(sql,"SELECT ROOM_NAME,PEOPLE_NUMBER,PASSWORD FROM ROOM");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                post_list[0]='\0';
                while(sqlite3_step(DBout)==SQLITE_ROW){
                    strcat(post_list,"[VSS]Room Name: ");
                    strcat(post_list,sqlite3_column_text(DBout,0));
                    strcat(post_list," ### People: ");
                    valid=sqlite3_column_int(DBout,1);
                    sprintf(post,"%d",valid);
                    strcat(post_list,post);
                    strcat(post_list,"/10 ### ");
                    strcpy(post,sqlite3_column_text(DBout,2));
                    if(strlen(post)==1)
                        strcat(post_list,"NO PASSWORD\n");
                    else
                        strcat(post_list,"PASSWORD\n");
                }
                if(strlen(post_list)==0){
                    strcpy(post_list,"[VSS]No chat rooms are active\n");
                }
                if(send_data(client,post_list,strlen(post_list)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"createchat")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Name for the room? 8-30 EN characters\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(pal);
                if(checkValidUsername(pal)!=1){
                    strcpy(cmdmsg,"[VSS]Invalid name for the room\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(sql,"SELECT * FROM ROOM WHERE ROOM_NAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult==SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]Room with this name already exists\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                sqlite3_finalize(DBout);
                strcpy(cmdmsg,"[VSS]Password | 2 or less chars means no password\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,cmdmsg)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(cmdmsg);
                VALID_ACC;
                strcpy(sql,"INSERT INTO ROOM VALUES(\"");
                strcat(sql,pal);
                strcat(sql,"\",\"");
                if(strlen(cmdmsg)<3)
                    strcat(sql,"a");
                else
                    strcat(sql,cmdmsg);
                strcat(sql,"\",1,0);");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Welcome to chatrooms| Press Enter to refresh chat | /help in case you need help\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    CHATERR;
                }
                counter=0;
                strcpy(cmdmsg,username);
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    CHATERR;    
                }
                //CHAT
                while (1){
                    if(recv_data(client,chat_message)==-1){
                        CHATERR;
                    }
                    if(strcmp(chat_message,"/leave")==0){
                        strcpy(sql,"UPDATE ROOM SET PEOPLE_NUMBER=PEOPLE_NUMBER-1,MESSAGE_COUNT=MESSAGE_COUNT+1 WHERE ROOM_NAME=\"");
                        strcat(sql,pal);
                        strcat(sql,"\";");
                        DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                        if (DBresult != SQLITE_OK ) {
                            DBERROR;
                        }
                        strcpy(sql,"SELECT * FROM ROOM WHERE ROOM_NAME=\"");
                        strcat(sql,pal);
                        strcat(sql,"\" AND PEOPLE_NUMBER=0;");
                        DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                        if( DBresult != SQLITE_OK ){
                            DBERROR;
                        }
                        DBresult=sqlite3_step(DBout);
                        if(DBresult==SQLITE_ROW){
                            sqlite3_finalize(DBout);
                            strcpy(sql,"DELETE FROM CHAT WHERE ROOM_NAME=\"");
                            strcat(sql,pal);
                            strcat(sql,"\";");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                            strcpy(sql,"DELETE FROM ROOM WHERE PEOPLE_NUMBER=0;");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                        }
                        else{
                            sqlite3_finalize(DBout);
                            strcpy(sql,"SELECT MAX(MESAGE_NUMBER) FROM CHAT");
                            DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                            if( DBresult != SQLITE_OK ){
                                DBERROR;
                            }
                            DBresult=sqlite3_step(DBout);
                            counter=sqlite3_column_int(DBout,0)+1;
                            sqlite3_finalize(DBout);
                            strcpy(sql,"INSERT INTO CHAT VALUES(\"");
                            strcat(sql,username);
                            strcat(sql,"\",\"");
                            strcat(sql,pal);
                            strcat(sql,"\",\"");
                            strcat(sql,username);
                            strcat(sql," has left the chat!\",");
                            sprintf(post_list,"%d",counter);
                            strcat(sql,post_list);
                            strcat(sql,");");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                        }
                        strcpy(post_list,"[VSS]Left chatroom!\n");
                        if(send_data(client,post_list,strlen(post_list)+1)==-1){
                            CHATERR;
                        }
                        break;
                    }
                    else if(strcmp(chat_message,"/help")==0){
                        strcpy(post_list,HELPCHAT);
                        if(send_data(client,post_list,strlen(post_list)+1)==-1){
                            CHATERR;
                        }
                    }
                    else{
                        strcpy(sql,"SELECT USERNAME,MESAGE,MESAGE_NUMBER FROM CHAT WHERE ROOM_NAME=\"");
                        strcat(sql,pal);
                        strcat(sql,"\" AND MESAGE_NUMBER>");
                        sprintf(post_list,"%d",counter);
                        strcat(sql,post_list);
                        strcat(sql," ORDER BY MESAGE_NUMBER;");
                        DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                        if( DBresult != SQLITE_OK ){
                            DBERROR;
                        }
                        post_list[0]='\0';
                        while(sqlite3_step(DBout)==SQLITE_ROW){
                            strcat(post_list,"[");
                            strcat(post_list,sqlite3_column_text(DBout,0));
                            strcat(post_list,"]: ");
                            strcat(post_list,sqlite3_column_text(DBout,1));
                            strcat(post_list,"\n");
                            counter=counter+1;
                        }
                        sqlite3_finalize(DBout);
                        if(strlen(chat_message)>0){
                            CHECKFORDB(chat_message);
                            counter=counter+1;
                            sprintf(post,"%d",counter);
                            strcpy(sql,"INSERT INTO CHAT VALUES(\"");
                            strcat(sql,username);
                            strcat(sql,"\",\"");
                            strcat(sql,pal);
                            strcat(sql,"\",\"");
                            strcat(sql,chat_message);
                            strcat(sql,"\",");
                            strcat(sql,post);
                            strcat(sql,");");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                            strcpy(sql,"UPDATE ROOM SET MESSAGE_COUNT=MESSAGE_COUNT+1 WHERE ROOM_NAME=\"");
                            strcat(sql,pal);
                            strcat(sql,"\";");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                        }
                        if(send_data(client,post_list,strlen(post_list)+1)==-1){
                            CHATERR;
                        }
                    }
                }
                
            }
            else if(strcmp(cmdmsg,"joinchat")==0 && logged==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Insert the name of the chatroom\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(pal);
                strcpy(sql,"SELECT MESSAGE_COUNT,PASSWORD,PEOPLE_NUMBER,MESSAGE_COUNT FROM ROOM WHERE ROOM_NAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No such room exists\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                proceed=sqlite3_column_int(DBout,0);
                valid=sqlite3_column_int(DBout,2);
                strcpy(post,sqlite3_column_text(DBout,1));
                counter=sqlite3_column_int(DBout,3);
                sqlite3_finalize(DBout);
                if(valid>=10){
                    strcpy(cmdmsg,"[VSS]Chatroom is full\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                if(strlen(post)>2){
                    strcpy(cmdmsg,"[VSS]Enter the password\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    if(recv_data(client,cmdmsg)==-1){
                        FORCELOGOUT;
                    }
                    CHECKFORDB(pal);
                    if(strcmp(cmdmsg,post)!=0){
                        strcpy(cmdmsg,"[VSS]Incorrect password\n");
                        if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                            FORCELOGOUT;
                        }
                        continue;
                    }
                }
                VALID_ACC;
                strcpy(sql,"UPDATE ROOM SET PEOPLE_NUMBER=PEOPLE_NUMBER+1,MESSAGE_COUNT=MESSAGE_COUNT+1 WHERE ROOM_NAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                counter=counter+1;
                strcpy(sql,"INSERT INTO CHAT VALUES(\"");
                strcat(sql,username);
                strcat(sql,"\",\"");
                strcat(sql,pal);
                strcat(sql,"\",\"");
                strcat(sql,username);
                strcat(sql," has joined the chat!\",");
                sprintf(cmdmsg,"%d",counter);
                strcat(sql,cmdmsg);
                strcat(sql,");");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Welcome to chatrooms| Press Enter to refresh chat | /help in case you need help\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    CHATERR;
                }
                strcpy(cmdmsg,username);
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    CHATERR;
                }
                //CHAT
                while (1){
                    if(recv_data(client,chat_message)==-1){
                        CHATERR;
                    }
                    if(strcmp(chat_message,"/leave")==0){
                        strcpy(sql,"UPDATE ROOM SET PEOPLE_NUMBER=PEOPLE_NUMBER-1,MESSAGE_COUNT=MESSAGE_COUNT+1 WHERE ROOM_NAME=\"");
                        strcat(sql,pal);
                        strcat(sql,"\";");
                        DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                        if (DBresult != SQLITE_OK ) {
                            DBERROR;
                        }
                        strcpy(sql,"SELECT * FROM ROOM WHERE ROOM_NAME=\"");
                        strcat(sql,pal);
                        strcat(sql,"\" AND PEOPLE_NUMBER=0;");
                        DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                        if( DBresult != SQLITE_OK ){
                            DBERROR;
                        }
                        DBresult=sqlite3_step(DBout);
                        if(DBresult==SQLITE_ROW){
                            sqlite3_finalize(DBout);
                            strcpy(sql,"DELETE FROM CHAT WHERE ROOM_NAME=\"");
                            strcat(sql,pal);
                            strcat(sql,"\";");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                            strcpy(sql,"DELETE FROM ROOM WHERE PEOPLE_NUMBER=0;");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                        }
                        else{
                            sqlite3_finalize(DBout);
                            strcpy(sql,"SELECT MAX(MESAGE_NUMBER) FROM CHAT");
                            DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                            if( DBresult != SQLITE_OK ){
                                DBERROR;
                            }
                            DBresult=sqlite3_step(DBout);
                            counter=sqlite3_column_int(DBout,0)+1;
                            sqlite3_finalize(DBout);
                            strcpy(sql,"INSERT INTO CHAT VALUES(\"");
                            strcat(sql,username);
                            strcat(sql,"\",\"");
                            strcat(sql,pal);
                            strcat(sql,"\",\"");
                            strcat(sql,username);
                            strcat(sql," has left the chat!\",");
                            sprintf(post_list,"%d",counter);
                            strcat(sql,post_list);
                            strcat(sql,");");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                        }
                        strcpy(post_list,"[VSS]Left chatroom!\n");
                        if(send_data(client,post_list,strlen(post_list)+1)==-1){
                            CHATERR;
                        }
                        break;
                    }
                    else if(strcmp(chat_message,"/help")==0){
                        strcpy(post_list,HELPCHAT);
                        if(send_data(client,post_list,strlen(post_list)+1)==-1){
                            CHATERR;
                        }
                    }
                    else{
                        strcpy(sql,"SELECT USERNAME,MESAGE,MESAGE_NUMBER FROM CHAT WHERE ROOM_NAME=\"");
                        strcat(sql,pal);
                        strcat(sql,"\" AND MESAGE_NUMBER>");
                        sprintf(post_list,"%d",counter);
                        strcat(sql,post_list);
                        strcat(sql," ORDER BY MESAGE_NUMBER;");
                        DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                        if( DBresult != SQLITE_OK ){
                            DBERROR;
                        }
                        post_list[0]='\0';
                        while(sqlite3_step(DBout)==SQLITE_ROW){
                            strcat(post_list,"[");
                            strcat(post_list,sqlite3_column_text(DBout,0));
                            strcat(post_list,"]: ");
                            strcat(post_list,sqlite3_column_text(DBout,1));
                            strcat(post_list,"\n");
                            counter=counter+1;
                        }
                        sqlite3_finalize(DBout);
                        if(strlen(chat_message)>0){
                            CHECKFORDB(chat_message);
                            counter=counter+1;
                            sprintf(post,"%d",counter);
                            strcpy(sql,"INSERT INTO CHAT VALUES(\"");
                            strcat(sql,username);
                            strcat(sql,"\",\"");
                            strcat(sql,pal);
                            strcat(sql,"\",\"");
                            strcat(sql,chat_message);
                            strcat(sql,"\",");
                            strcat(sql,post);
                            strcat(sql,");");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                            strcpy(sql,"UPDATE ROOM SET MESSAGE_COUNT=MESSAGE_COUNT+1 WHERE ROOM_NAME=\"");
                            strcat(sql,pal);
                            strcat(sql,"\";");
                            DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                            if (DBresult != SQLITE_OK ) {
                                DBERROR;
                            }
                        }
                        if(send_data(client,post_list,strlen(post_list)+1)==-1){
                           CHATERR;
                        }
                    }
                }
            }
            else if(strcmp(cmdmsg,"deletepost")==0 && admin==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]What id does the post have?\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,cmdmsg)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(cmdmsg);
                if(checkRowID(cmdmsg)==-1){
                    strcpy(cmdmsg,"[VSS]Id format unkown\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                strcpy(sql,"SELECT * FROM POST WHERE ROWID=");
                strcat(sql,cmdmsg);
                strcat(sql,";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No such post id found\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                sqlite3_finalize(DBout);
                VALID_ACC;
                strcpy(sql,"DELETE FROM POST WHERE ROWID=");
                strcat(sql,cmdmsg);
                strcat(sql,";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Post deleted\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(strcmp(cmdmsg,"deleteaccount")==0 && admin==1){
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]What username does the account have?\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
                if(recv_data(client,pal)==-1){
                    FORCELOGOUT;
                }
                CHECKFORDB(pal);
                strcpy(sql,"SELECT * FROM USER WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_prepare_v2(db, sql, -1, &DBout, 0);
                if( DBresult != SQLITE_OK ){
                    DBERROR;
                }
                DBresult=sqlite3_step(DBout);
                if(DBresult!=SQLITE_ROW){
                    sqlite3_finalize(DBout);
                    strcpy(cmdmsg,"[VSS]No such account found\n");
                    if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                        FORCELOGOUT;
                    }
                    continue;
                }
                sqlite3_finalize(DBout);
                strcpy(sql,"DELETE FROM POST WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(sql,"DELETE FROM FRIEND WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\" OR PAL=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(sql,"DELETE FROM REQUEST WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\" OR PAL=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(sql,"DELETE FROM USER WHERE USERNAME=\"");
                strcat(sql,pal);
                strcat(sql,"\";");
                DBresult=sqlite3_exec(db, sql, 0, 0, &errmsg);
                if (DBresult != SQLITE_OK ) {
                    DBERROR;
                }
                strcpy(cmdmsg,"[VSS]Account has been deleted\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else if(logged==1) {
                VALID_ACC;
                strcpy(cmdmsg,"[VSS]Command not found! Try \"help\" for a list of useful commands\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)==-1){
                    FORCELOGOUT;
                }
            }
            else{
                strcpy(cmdmsg,"[VSS]Command not found! Try \"help\" for a list of useful commands\n");
                if(send_data(client,cmdmsg,strlen(cmdmsg)+1)){
                    SENDERR;
                }
            }
            if(signal(SIGPIPE,signal_handler_wr)==SIG_ERR){ 
                printf("[VSS]Could not catch signal\n");
            } 
        }
        free(clientIP);
        sqlite3_close(db);
        close(client);
        exit(1);
    }
    //Server
    close(client);
    printf("[VSS]Detaching done!\n");

    //Handle finished svChildren
    if(signal(SIGCHLD,signal_handler)==SIG_ERR){
        printf("[VSS]Could not catch signal\n");
    }	
}
}
///SOLVE SIGPIPE

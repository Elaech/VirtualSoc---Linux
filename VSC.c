#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#define PORT 5174
extern int errno;

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
    if(read(fd,&size,sizeof(size))==-1){
        perror("Error in receiving message\n");
        return -1;
    }
    if(read(fd,&key,sizeof(key))==-1){
        perror("Error in receiving message\n");
        return -1;
    }
    key=key-size;
    if(read(fd,buff,size)==-1){
        perror("Error in receiving message\n");
        return -1;
    }
    decrypt(buff,key);
    return size;
}

int main(int argc,char * argv[]){

//testing arguments
if(argc!=2){
    printf("[VSC]Please insert only the server-ip\n");
    return 0;
}

//socket
int sd;	
if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
    perror ("Error in creating socket\n");
    return errno;
}
//server
struct sockaddr_in server; 
server.sin_family = AF_INET;
server.sin_addr.s_addr = inet_addr(argv[1]);
server.sin_port = htons (PORT);
if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1){
      perror ("[VSC]Eroare la connect().\n");
      return errno;
}
printf("Connection Established\n");
///////Communication Data//////////////////////
char post_list[16384];
int size;
char username[35];
///////////////////////////////////////////////
while(1){
    fflush(stdout);
    printf("[VSC]Input:\n");
    while((size=read(0,post_list,sizeof(post_list)))<=0){
        printf("[VSC]Error in reading your command. Please try again\n");
        fflush(stdout);
    }
    post_list[size-1]='\0';
    send_data(sd,post_list,strlen(post_list)+1);
    //Public Users
    recv_data(sd,post_list);
    //disconnect
    if(strcmp(post_list,"quit")==0){
        printf("[VSC]Disconnected\n");
        break;
    }
    //error account
    if(strcmp(post_list,"[VSS]Your account has been deleted\n")==0){
        printf("%s",post_list);
        printf("[VSC]Disconnected\n");
        break;
    }
    //password
    else if(strcmp(post_list,"[VSS]Enter desired password (8-30 must have small and big letters and numbers) |\"cancel\" to cancel the proccess\n")==0
            || strcmp(post_list,"[VSS]Password:\n")==0){
        printf("%s",post_list);
        while(1){
        struct termios old, new;
        if (tcgetattr (fileno (stdin), &old) != 0)
            return -1;
        new = old;
        new.c_lflag &= ~ECHO;
        if (tcsetattr (fileno (stdin), TCSAFLUSH, &new) != 0)
            return -1;
        while((size=read(0,post_list,sizeof(post_list)))<=0){
        printf("[VSC]Error in reading your command. Please try again\n");
        fflush(stdout);
        }
        (void) tcsetattr (fileno (stdin), TCSAFLUSH, &old);
        post_list[size-1]='\0';
        send_data(sd,post_list,size);
        recv_data(sd,post_list);
        printf("%s",post_list);
        if(strcmp(post_list,"[VSS]Account successfuly created!Try to log in\n")==0 || strcmp(post_list,"[VSS]Successfuly cancelled!\n")==0 
            || strcmp(post_list,"[VSS]Successfuly logged into your account\n")==0 || strcmp(post_list,"[VSS]Username or password are incorrect\n")==0
            || strcmp(post_list,"[VSS]Account already logged in\n")==0)
            break;
        }
    }
    //CHAT ROOM
    else if(strcmp(post_list,"[VSS]Welcome to chatrooms| Press Enter to refresh chat | /help in case you need help\n")==0){
        printf("%s",post_list);
        recv_data(sd,username);
        while(1){
            fflush(stdout);
            printf("[%s]:",username);
            fflush(stdout);
            while((size=read(0,post_list,sizeof(post_list)))<=0){
                printf("[VSC]Error in reading your command. Please try again\n");
                fflush(stdout);
            }
            post_list[size-1]='\0';
            send_data(sd,post_list,strlen(post_list)+1);
            recv_data(sd,post_list);
            if(strlen(post_list)!=0)
                printf("%s",post_list);
            if(strcmp(post_list,"[VSS]Left chatroom!\n")==0)
                break;
        }
    }
    else
        printf("%s",post_list);
}
close(sd);
}


/** Chat Service Project
 * 
 * Author: John Roussos
 * License: MIT
 * 
 */ 

#include <stdio.h>
#include <string.h>  
#include <stdlib.h>   
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>    
#include <pthread.h> 
#include <semaphore.h>

//color codes 
#define NRM  "\x1B[0m"
#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"
#define YEL  "\x1B[33m"
#define BLU  "\x1B[34m"
#define MAG  "\x1B[35m"
#define CYN  "\x1B[36m"
#define WHT  "\x1B[37m"
#define RESET "\033[0m"

 
#define PORT 8080
#define lineSize 40

sem_t semaphore;

int usersListSize = 1, 
onlineUserListSize = 0, 
userChatingListSize = 0, 
groupListSize = 0,
sizeOfUsersInGroup = 10;

typedef struct userBluePrint{
    char username[20];
    char password[10];
}user;

typedef struct onlineUsersList{
    char username[20];
    int socket;
}onlineUser;

typedef struct chatingUsers{
    char username[20];
    char chatingWithUser[20];
    char chatingWithGroup[20];
}userChatsWith;

typedef struct groupBlueprint{
    char groupName[10];
    char creator[20];
    char listOfUsers[10][20];
}group;

user *usersList;
onlineUser *onlineUserList;
userChatsWith *chatingList;
group *groupList;
FILE *fp, *fp_temp;

struct sockaddr_in server , client;

void sendToallOnlineUsers(int sock, char receivedMessage[]);
void *connection_handler(void *);
void *server_info();
int singIn(user potentialUser);
int logIn(user PotentialUser);
void printDB();
void printOnline();
void printChat();
void printGroups();
void sendContactsList(int socket, char username[]);
void sendGroupsList(int socket, char username[]);
void createGroup(char groupName[], char username[]);
void createNewGroup(char username[], char groupName[]);
void manageGroup(int socket, char username[]);
void deleteGroup(int socket, char username[]);
int chatWithContact(int socket, char contact[], char username[]);
int chatWithGroup(int socket[], char group[], char username[]);
void addContact(char whatToAdd[], int socket, char userToAdd[], char username[]);
void checkingIfThereIsASavedMessage(int socketArray[], int socket, char clientName[], char chatee[]);

 
int main(int argc , char *argv[]){
    sem_init(&semaphore, 0, 1); 
    usersList = malloc(usersListSize*sizeof(user));///might not needed will have to recheck it!!
    chatingList = malloc(sizeof(userChatsWith));
    groupList = malloc(sizeof(group));
    int socket_desc , client_sock , c;

    if(usersList == NULL){
        perror("ERROR: create dynamic list\n");
        exit(EXIT_FAILURE);
    }

    if((fp = fopen("users.list", "a+")) == NULL ){
        perror("ERROR: Cannot read users from file\n");
        exit(EXIT_FAILURE);
    }
    char *line, bufferedLine[lineSize];
    int i=0, k=0;

    //read list of users from file and inserting them to an array 
    while (fgets(bufferedLine, sizeof(bufferedLine), fp) != NULL){
        bufferedLine[strlen(bufferedLine) - 1] = '\0'; //get rid of the \n that fgets pushes
        line = strtok(bufferedLine, "|");

        usersListSize++;
        if ( (usersList = realloc(usersList, usersListSize*sizeof(user))) == NULL ){
            printf("error");
            exit(EXIT_FAILURE);
        }

        strcpy(usersList[i].username, line);
        // printf("username: %s  ", line);

        line = strtok(NULL, "|");
        strcpy(usersList[i].password, line);
        // printf("password: %s\n", line);
        i++;
    }
    fclose(fp);
    
    //read list of groups from file and inserting them to an array
    if((fp = fopen("groups.list", "a+")) == NULL ){
        perror("ERROR: Cannot read groups from file\n");
        exit(EXIT_FAILURE);
    }
    while(fgets(bufferedLine, sizeof(bufferedLine), fp)){
        bufferedLine[strlen(bufferedLine) - 1] = '\0';

        groupListSize++;
        groupList = realloc(groupList, groupListSize*sizeof(group));

        line = strtok(bufferedLine, ":");
        strcpy(groupList[groupListSize-1].groupName, line);

        line = strtok(NULL, ":");
        strcpy(groupList[groupListSize-1].creator, line);

        line = strtok(NULL, "|");
        for(int k=0; k<10; k++){
            if(line == NULL)
                break;
            strcpy(groupList[groupListSize-1].listOfUsers[k], line);
            line = strtok(NULL, "|");
        }
    }

    fclose(fp);
   
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );
     
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        perror("ERROR: bind failed");
        return 1;
    }
    puts("Bind done");
     
    listen(socket_desc , 3);     
     
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t traffic_thread, server_thread;
	
    printf(CYN"\nYou can type 'info' to see the status of database or 'help' to see the full list of commands and 'exit' to terminate server\n" RESET);
    if( pthread_create( &server_thread , NULL , server_info , NULL) < 0){
        perror("could not create thread");
        return 1;
    }
    
    //for every new connection creates an new thread to handle it
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        puts(GRN "Connection accepted" RESET);
         
        if( pthread_create( &traffic_thread , NULL ,  connection_handler , (void*) &client_sock) < 0){
            perror("Could not create thread");
            return 1;
        }
         
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0){
        perror("accept failed");
        return 1;
    }
    sem_destroy(&semaphore);
    return 0;
}

void *server_info(){//thread that handles the commands typed to server
    char command[10];
    while(1){
        scanf("%s", command);

        if(strcmp(command, "db") == 0)
            printDB();
        else if(strcmp(command, "online") == 0)
            printOnline();
        else if(strcmp(command, "exit") == 0)
            exit(EXIT_SUCCESS);
        else if(strcmp(command, "chat") == 0){
            printChat();
        }else if(strcmp(command, "groups") == 0){
            printGroups();
        }else if(strcmp(command, "info") == 0){
            printDB();
            printOnline();
            printChat();
            printGroups();
        }else if(strcmp(command, "help") == 0){
            printf("\n\n'info'\t\tprints the full status of the database\n'db'\t\tprints all the users registered in the app\n'online'\tprints the list of online users\n'chat'\t\tprints which user is chating with which\n'groups'\tprints a list with every group\n");
        }
        
    }
    pthread_exit(NULL);
}
 
void *connection_handler(void *socket_desc){//thread that handles all the traffic
    int isThisTheFirstTime = 1;
    int sock = *(int*)socket_desc;
    int read_size;
    int respond;
    int sockToChat;
    int socketArrayForGroupChat[sizeOfUsersInGroup];
    char *message , client_message[2000], sendable_message[2024];
    user temp;
    
    char *p, *action, *command;
    
    //varification of the username and password given from the client 
    // authorize user (1. success - 2. failure)
    while(respond != 1){
        printf(MAG "NEW CONNECTION: %s:%d " RESET , inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        puts(RESET);
        read_size = recv(sock , client_message , 2000 , 0);

        if(read_size == 0){
            printf(RED "Client disconnected\n" RESET);
            fflush(stdout);
            pthread_exit(NULL);
        }
        else if(read_size == -1){
            perror("recv failed");
            pthread_exit(NULL);
        }

        //end of string marker
        client_message[read_size] = '\0';

        p = strtok (client_message, "|");
        action = p;
        if( strcmp(action, "sig") == 0){
            //search in the user list for existing user with the same name and if not exists then continue
            strcpy(temp.username, strtok (NULL, "|"));
            strcpy(temp.password, strtok (NULL, "|"));
            respond = singIn(temp);
        }else if( strcmp(action, "log") == 0){
            //search in the user list for the user and the password and if match then continue
            strcpy(temp.username, strtok (NULL, "|"));
            strcpy(temp.password, strtok (NULL, "|"));
            respond = logIn(temp);
        }

        write(sock , &respond , sizeof(int));
            
        memset(client_message, 0, 2000);
    }//end of the verification process 

    //dynamically resize the user list and adding another user
    onlineUserListSize++;
    if( (onlineUserList = realloc(onlineUserList, onlineUserListSize*sizeof(onlineUser))) == NULL){
        perror("ERROR: dynamic list");
        exit(0);
    }
    onlineUserList[onlineUserListSize-1].socket = sock;
    strcpy(onlineUserList[onlineUserListSize-1].username, temp.username);
 
    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
        client_message[read_size] = '\0';//end of string marker

        char clientName[20], name[20];
        
        //recognising that the client has sent a command instead of a message
        if(client_message[0] == '$'){
            command = strtok(client_message, ":");
            if(strcmp(command, "$contacts") == 0){
                sendContactsList(sock, temp.username);
            }else if(strcmp(command, "$groups") == 0){
                sendGroupsList(sock, temp.username);
            }else if(strcmp(command, "$setGroup") == 0){
                command = strtok(NULL, "");
                createNewGroup(temp.username, command);
            }else if(strcmp(command, "$manageGroup") == 0){
                deleteGroup(sock, temp.username);
            }else if(strcmp(command, "$contactChat") == 0){
                command = strtok(NULL, "");
                strcpy(clientName, command);
                sockToChat = chatWithContact(sock, command, temp.username);
            }else if(strcmp(command, "$contactGroup") == 0){
                command = strtok(NULL, "");
                strcpy(clientName, command);                
                sockToChat = chatWithGroup(socketArrayForGroupChat, command, temp.username);
            }else if(strcmp(command, "$addFriend") == 0){
                command = strtok(NULL, "");
                addContact("friend", sock, command, temp.username);
            }else if(strcmp(command, "$addGroup") == 0){
                command = strtok(NULL, "");
                addContact("group", sock, command, temp.username);
            }
        }else{
            for(int i=0; i<onlineUserListSize; i++){
                if(strcmp(onlineUserList[i].username, clientName) == 0){
                    for(int l=0; l<userChatingListSize; l++){
                        if(strcmp(chatingList[l].username, clientName) == 0){
                            if(strcmp(chatingList[l].chatingWithUser, temp.username) == 0){
                                sockToChat = onlineUserList[i].socket;
                            }
                        }
                    }
                }
            }
            for(int i=0; i<groupListSize; i++){
                if(strcmp(groupList[i].groupName, clientName) == 0){
                    for(int k=0; k<sizeOfUsersInGroup; k++){
                        for(int l=0; l<onlineUserListSize; l++){
                            if(strcmp(onlineUserList[l].username, groupList[i].listOfUsers[k]) == 0){
                                for(int p=0; p<userChatingListSize; p++){
                                    if(strcmp(chatingList[p].username, groupList[i].listOfUsers[k]) == 0){
                                        if(strcmp(chatingList[p].chatingWithGroup, groupList[i].groupName) == 0){
                                            socketArrayForGroupChat[k] = onlineUserList[l].socket;
                                            printf("%d\n", socketArrayForGroupChat[k]);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            printf("sockToChat: %d\n",sockToChat);
            if(sockToChat > 3){//chating with user
                printf("received: %s -> fd: %d\n", client_message, sockToChat);

                for(int i=0; i<userChatingListSize; i++){
                    if(strcmp(chatingList[i].username, clientName) == 0){
                        strcpy(name, chatingList[i].chatingWithUser);
                    }
                }
                
                printf("name: %s\n", name);
                checkingIfThereIsASavedMessage(NULL, sock, temp.username, name);

                strcat(sendable_message, MAG"(");
                strcat(sendable_message, name);
                strcat(sendable_message, "): "RESET);
                strcat(sendable_message, client_message);

                send(sockToChat, sendable_message, strlen(sendable_message), 0);
            }else if(sockToChat == -2){//chating with group
                printf("received: %s -> group: %s\n", client_message, clientName);
                
                if(isThisTheFirstTime){
                    checkingIfThereIsASavedMessage(socketArrayForGroupChat, 0, clientName, "$group");
                    isThisTheFirstTime = 0;
                }
                
                strcat(sendable_message, MAG"(");
                strcat(sendable_message, temp.username);
                strcat(sendable_message, "): "RESET);
                strcat(sendable_message, client_message);

                sem_wait(&semaphore);

                if((fp = fopen("savedMessages.list", "a+")) == NULL ){
                    perror("ERROR: Cannot read users from file\n");
                    exit(EXIT_FAILURE);
                }

                int isOnline;
                for(int i=0; i<sizeOfUsersInGroup; i++){
                    if(socketArrayForGroupChat[i] == sock)
                        continue;
                    send(socketArrayForGroupChat[i], sendable_message, strlen(sendable_message), 0);
                    for(int k=0; k<onlineUserListSize; k++){
                        if(socketArrayForGroupChat[i] == onlineUserList[k].socket)
                            isOnline = 1;
                        else
                            isOnline = 0;
                    }
                }
                if(isOnline == 0)
                    fprintf(fp, "%s|%s:%s\n", clientName, temp.username, client_message);
                fclose(fp);

                sem_post(&semaphore);
            }else if(sockToChat == -1){
                checkingIfThereIsASavedMessage(NULL, sock, temp.username, clientName);
                printf("received: %s -> fd: %d\n", client_message, sockToChat);

                sem_wait(&semaphore);

                if((fp = fopen("savedMessages.list", "a+")) == NULL ){
                    perror("ERROR: Cannot read users from file\n");
                    exit(EXIT_FAILURE);
                }
                fprintf(fp, "%s|%s:%s\n", clientName, temp.username, client_message);
                fclose(fp);
                
                sem_post(&semaphore);
                printf("client not online. Message is saved\n");
            }
        }

        //send message to all clients
        //sendToallOnlineUsers(sock, client_message);
        
        //clear the message buffer
        memset(client_message, 0, 2000);
        memset(sendable_message, 0, 2024);
    }
     
    if(read_size == 0){
        printf(RED "Client disconnected\n" RESET);
        fflush(stdout);
        for(int i=0; i<onlineUserListSize; i++){
            if(strcmp(onlineUserList[i].username, temp.username) == 0){
                strcpy(onlineUserList[i].username, onlineUserList[onlineUserListSize-1].username);
                onlineUserList[i].socket = onlineUserList[onlineUserListSize-1].socket;
                onlineUserListSize--;
            }
        }

        for(int i=0; i<userChatingListSize; i++){
            if(strcmp(chatingList[i].username, temp.username) == 0){
                strcpy(chatingList[i].username, chatingList[userChatingListSize-1].username);
                strcpy(chatingList[i].chatingWithUser, chatingList[userChatingListSize-1].chatingWithUser);
                strcpy(chatingList[i].chatingWithGroup, chatingList[userChatingListSize-1].chatingWithGroup);
                userChatingListSize--;
            }
        }
        
        close(sock);
    }
    else if(read_size == -1){
        perror("recv failed");
    }
         
    pthread_exit(NULL);
} 

int singIn(user potentialUser){
    //searching
    for(int i=0; i<usersListSize; i++){
        if(strcmp(usersList[i].username, "") != 0){ //cell is not empty
            if(strcmp(usersList[i].username, potentialUser.username) == 0){
                printf("user already exists\n");
                return 0; //error user already exists
            }  
        }else{ //cell is empty
            usersListSize++;
            if ( (usersList = realloc(usersList, usersListSize*sizeof(user))) == NULL ){
                perror("ERROR: dynamic list");
                exit(0);
            }
            strcpy(usersList[i].username, potentialUser.username);
            strcpy(usersList[i].password, potentialUser.password);
            printf("%s|%s\n", potentialUser.username, potentialUser.password);

            sem_wait(&semaphore);
            if((fp = fopen("users.list", "a+")) == NULL ){
                perror("ERROR: Cannot read file\n");
                exit(EXIT_FAILURE);
            }
            fprintf(fp, "%s|%s\n", potentialUser.username, potentialUser.password);
            fclose(fp);

            if((fp = fopen("contacts.list", "a+")) == NULL ){
                perror("ERROR: Cannot read groups from file\n");
                exit(EXIT_FAILURE);
            }
            fprintf(fp, "%s:\n", potentialUser.username);
            fclose(fp);

            sem_post(&semaphore);
            break;
        }
    }
    return 1; //success
}

int logIn(user potentialUser){
    for(int i=0; i<usersListSize; i++){
        for(int l=0; l<onlineUserListSize; l++){
            if(strcmp(onlineUserList[l].username, potentialUser.username) == 0){
                return 2; //already logged in
            }
        }
        if(strcmp(usersList[i].username, potentialUser.username) == 0 && 
           strcmp(usersList[i].password, potentialUser.password) == 0){
               return 1; //success
        }
    }
    return 0; //error username or password dont match
}

void sendToallOnlineUsers(int sock, char receivedMessage[]){
    for(int i=0; i<onlineUserListSize; i++){
        if(onlineUserList[i].socket == sock)
            continue;
        send(onlineUserList[i].socket, receivedMessage, strlen(receivedMessage),0);
    }    
}

void printDB(){
    /** print the user list 
     */
    printf("\n\nDatabase\n");
    printf("----------------------------\n");
        for(int i=0; i<usersListSize-1; i++){
            printf("Entry %2d - user: %s  pass: %s\n", i+1, usersList[i].username, usersList[i].password);
        }
    printf("----------------------------\n\n");
}

void printOnline(){
    /** print online users list
     */
    printf("\n\nOnline\n");
    printf("----------------------------\n");
    for(int i=0; i<onlineUserListSize; i++){
        printf("Online %2d - user: %s  fd: %d\n", i+1, onlineUserList[i].username, onlineUserList[i].socket);
    }
    printf("----------------------------\n\n");
}

void printChat(){
    /** print which user is chating with which 
     */
    printf("\n\nChat\n");
    printf("----------------------------\n");
    for(int i=0; i<userChatingListSize; i++){
        printf("Entry %2d - user: %s\tchatingWithUser: %s\tchatingWithGroup: %s\n", i+1, chatingList[i].username, chatingList[i].chatingWithUser, chatingList[i].chatingWithGroup);
    }
    printf("----------------------------\n\n");
}

void printGroups(){
    /** print the groups 
     */
    printf("\n\nGroups\n");
    printf("----------------------------\n");
    for(int i=0; i<groupListSize; i++){
        printf("Entry %2d - group name: %s  creator: %s  user list: ", i+1, groupList[i].groupName, groupList[i].creator);
        for(int k=0; k<10; k++){
            // fputs(groupList[i].listOfUsers[k], stdout);
            printf("%s ", groupList[i].listOfUsers[k]);
        }
        printf("\n");
    }
    printf("----------------------------\n\n");
}

void sendContactsList(int socket, char username[]){
    char *line, *rest, bufferedLine[60], sendableLine[60];
    sem_wait(&semaphore);
    if((fp = fopen("contacts.list", "a+")) == NULL ){
        perror("ERROR: Cannot read contacts from file\n");
        exit(EXIT_FAILURE);
    }
    while(fgets(bufferedLine, sizeof(bufferedLine), fp) != NULL){
        bufferedLine[strlen(bufferedLine) - 1] = '\0';
        rest = strchr(bufferedLine, ':');
        line = strtok(bufferedLine, ":");

        strcpy(sendableLine, rest+1);
        // printf("size: %ld\n", strlen(sendableLine));

        if(strcmp(line, username) == 0){
            if(strlen(sendableLine) <= 1)
                send(socket, "noContacts", 11,0);
            else
                send(socket, sendableLine, strlen(sendableLine), 0);
        }
    }
    fclose(fp);
    sem_post(&semaphore);
}

void sendGroupsList(int socket, char username[]){
    char *line, *rest, bufferedLine[60], sendableLine[60];
    memset(sendableLine, 0, 60);
    for(int i=0; i<groupListSize; i++){
        for(int k=0; k<sizeOfUsersInGroup; k++){
            if(strcmp(groupList[i].listOfUsers[k], username) == 0){
                strcat(sendableLine, groupList[i].groupName);
                strcat(sendableLine, "|");
            }
        }
    }
    // printf("line: %s - size: %ld\n", sendableLine, strlen(sendableLine));
    if(strlen(sendableLine) <= 1)
        send(socket, "noGroups", 9,0);
    else
        send(socket, sendableLine, strlen(sendableLine)-1, 0);
}

void createGroup(char username[], char groupName[]){
    char line[55], *prefix;

    printf("Create Group: %s\n", groupName);

    sem_wait(&semaphore);

    if((fp = fopen("groups.list", "a+")) == NULL ){
        perror("ERROR: Cannot read groups from file\n");
        exit(EXIT_FAILURE);
    }
    if((fp_temp = fopen("temp", "a+")) == NULL ){
        perror("ERROR: Cannot read groups from file\n");
        exit(EXIT_FAILURE);
    }

    while(fscanf(fp, "%s\n", line) != EOF){
        prefix = strtok(line, ":");
        if(strcmp(prefix, username) == 0){
            prefix = strtok(NULL, "");
            if(prefix == NULL){
                fprintf(fp_temp, "%s:%s\n", username, groupName);
            }else{
                fprintf(fp_temp, "%s:%s|%s\n", username, prefix, groupName);
            }
            continue;
        }
        fprintf(fp_temp, "%s:\n", line);
    }


    fclose(fp);
    fclose(fp_temp);
    remove("groups.list");
    rename("temp", "groups.list");

    sem_post(&semaphore);
}

void createNewGroup(char username[], char groupName[]){
    printf("Create Group: %s\n", groupName);
    
    groupListSize++;
    groupList = realloc(groupList, groupListSize*sizeof(group));

    strcpy(groupList[groupListSize-1].groupName, groupName);
    strcpy(groupList[groupListSize-1].creator, username);
    strcpy(groupList[groupListSize-1].listOfUsers[0], username);

    sem_wait(&semaphore);

    if((fp = fopen("groups.list", "w")) == NULL ){
        perror("ERROR: Cannot read groups from file\n");
        exit(EXIT_FAILURE);
    }
    for(int i=0; i<groupListSize; i++){
        fprintf(fp, "%s:%s:", groupList[i].groupName, groupList[i].creator);
        for(int k=0; k<10; k++){
            if(groupList[i].listOfUsers[k][0] != '\0'){
                if(k != 0){
                    fprintf(fp, "|%s", groupList[i].listOfUsers[k]);
                }
                fprintf(fp, "%s", groupList[i].listOfUsers[k]);
            }
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    sem_post(&semaphore);
}

void deleteGroup(int socket, char username[]){
    char line[60], groupToDelete[10];

    memset(line, 0, 60); memset(groupToDelete, 0, 10);

    for(int i=0; i<groupListSize; i++){
        if(strcmp(groupList[i].creator, username) == 0){
            strcat(line, groupList[i].groupName);
            strcat(line, "|");
        }
    }
    // printf("%s")
    if(line[0] == '\0'){
        printf("No Groups\n");
        send(socket, "nogroups", 9, 0);
    }else{
        printf("Yes Groups\n");
        send(socket, line, strlen(line), 0);

        recv(socket, groupToDelete, 10, 0);
        
        sem_wait(&semaphore);
        if((fp = fopen("groups.list", "w")) == NULL ){
            perror("ERROR: Cannot read groups from file\n");
            exit(EXIT_FAILURE);
        }

        for(int i=0; i<groupListSize; i++){
            printf("groupName -> %s\n",groupList[i].groupName);
            if((strcmp(groupList[i].creator, username) == 0) && (strcmp(groupList[i].groupName, groupToDelete) == 0)){
                groupList[i] = groupList[groupListSize-1];
                groupListSize--;

                if(groupListSize > 0){
                    for(int i=0; i<groupListSize; i++){
                        fprintf(fp, "%s:%s:", groupList[groupListSize].groupName, groupList[groupListSize].creator);
                        for(int k=0; k<10; k++){
                            fprintf(fp, "%s|", groupList[groupListSize].listOfUsers[k]);
                        }
                    }
                }
            }
        }

        fclose(fp);
        sem_post(&semaphore);
    }
}

int chatWithContact(int socket, char contact[], char username[]){
    int socket_fd = 0; 

    //dynamically resize the chating list
    //this is a list that contains the online users and with whom their chating
    //it ensures that if a user tries to send a message to a user that is already chating with another one, that message will not be send during their session
    userChatingListSize++;
    if( (chatingList = realloc(chatingList, userChatingListSize*sizeof(userChatsWith))) == NULL){
        printf("error");
        exit(EXIT_FAILURE);
    }

    strcpy(chatingList[userChatingListSize-1].username, username);
    strcpy(chatingList[userChatingListSize-1].chatingWithUser, contact);

    for(int i=0; i<onlineUserListSize; i++){
        if(strcmp(onlineUserList[i].username, contact) == 0){
            for(int l=0; l<userChatingListSize; l++){
                if(strcmp(chatingList[l].username, contact) == 0){
                    if(strcmp(chatingList[l].chatingWithUser, username) == 0){
                        socket_fd = onlineUserList[i].socket;
                    }
                }
            }
            // printf("%d\n", socket_fd);
        }
    }

    if(socket_fd == 0){
        socket_fd = -1;
    }

    return socket_fd;
}

int chatWithGroup(int socket[], char group[], char username[]){
    int socket_fd = 0; 

    userChatingListSize++;
    if( (chatingList = realloc(chatingList, userChatingListSize*sizeof(userChatsWith))) == NULL){
        printf("error");
        exit(EXIT_FAILURE);
    }

    strcpy(chatingList[userChatingListSize-1].username, username);
    strcpy(chatingList[userChatingListSize-1].chatingWithGroup, group);

    for(int i=0; i<groupListSize; i++){
        if(strcmp(groupList[i].groupName, group) == 0){
            for(int k=0; k<sizeOfUsersInGroup; k++){
                for(int l=0; l<onlineUserListSize; l++){
                    if(strcmp(onlineUserList[l].username, groupList[i].listOfUsers[k]) == 0){
                        for(int p=0; p<userChatingListSize; p++){
                            if(strcmp(chatingList[p].username, groupList[i].listOfUsers[k]) == 0){
                                if(strcmp(chatingList[p].chatingWithGroup, groupList[i].groupName) == 0){
                                    socket[k] = onlineUserList[l].socket; //return an array with the sockets of the users in the group chat
                                    socket_fd = -2;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (socket_fd == 0){
        socket_fd = -1;
    }

    return socket_fd;
}

void addContact(char whatToAdd[], int socket, char userToAdd[], char username[]){
    int success = 0;
    char line[60], keepLine[60], *prefix;

    if(strcmp(whatToAdd, "friend") == 0){//add friend

        sem_wait(&semaphore);
        if((fp = fopen("contacts.list", "a+")) == NULL ){
            perror("ERROR: Cannot read groups from file\n");
            exit(EXIT_FAILURE);
        }
        if((fp_temp = fopen("temp", "a+")) == NULL ){
            perror("ERROR: Cannot read groups from file\n");
            exit(EXIT_FAILURE);
        }

        for(int i=0; i<usersListSize; i++){
            if(strcmp(userToAdd, usersList[i].username) == 0){//if user exist
                success = 1;
                while(fscanf(fp, "%s\n", line) != EOF){//reading from file
                    strcpy(keepLine, line); //printf("keepLine -> %s\n", keepLine);
                    prefix = strtok(line, ":"); //printf("prefix -> %s\n", prefix);
                    if(strcmp(prefix, username) == 0){
                        prefix = strtok(NULL, ""); //printf("prefix_2 -> %s\n", prefix);
                        if(prefix == NULL){
                            fprintf(fp_temp, "%s:%s\n", username, userToAdd);
                        }else{
                            fprintf(fp_temp, "%s:%s|%s\n", username, prefix, userToAdd);
                        }
                        continue;
                    }else{
                        fprintf(fp_temp, "%s\n", keepLine);
                    }
                }
                fclose(fp);
                fclose(fp_temp);
                remove("contacts.list");
                rename("temp", "contacts.list");

                sem_post(&semaphore);
                break;
            }
        }
        if(success)
            send(socket, "ok", 3, 0);
        else
            send(socket, "notok", 6, 0);
    }else{//add group
        for(int i=0; i<groupListSize; i++){
            if(strcmp(groupList[i].groupName, userToAdd) == 0){
                for(int k=0; k<sizeOfUsersInGroup; k++){
                    if(groupList[i].listOfUsers[k][0] == '\0'){
                        strcpy(groupList[i].listOfUsers[k], username);
                        success = 1;
                        break;
                    }
                }
            }
        }

        sem_wait(&semaphore);
        if((fp = fopen("groups.list", "w")) == NULL ){
            perror("ERROR: Cannot read groups from file\n");
            exit(EXIT_FAILURE);
        }

        if(success){
            send(socket, "ok", 3, 0);
            for(int i=0; i<groupListSize; i++){
                // rewind(fp);
                fprintf(fp, "%s:%s:", groupList[i].groupName, groupList[i].creator);
                for(int k=0; k<10; k++){
                    if(groupList[i].listOfUsers[k][0] != '\0'){
                        if(k != 0)
                            fprintf(fp, "|%s", groupList[i].listOfUsers[k]);
                        else
                            fprintf(fp, "%s", groupList[i].listOfUsers[k]);
                    }
                }
                fprintf(fp, "\n");
            }
        }else
            send(socket, "notok", 6, 0);

        fclose(fp);
        sem_post(&semaphore);
    }
}

void checkingIfThereIsASavedMessage(int socketArray[], int socket, char clientName[], char chatee[]){
    char line[60], *name, clientMessage[2024], keepline[60];
    memset(clientMessage, 0, 2024);
    printf("Checking..\n");
    printf("chatee: %s\n", chatee);

    sem_wait(&semaphore);
    if((fp = fopen("savedMessages.list", "a+")) == NULL ){
        perror("ERROR: Cannot read groups from file\n");
        exit(EXIT_FAILURE);
    }
    if((fp_temp = fopen("tempFile", "a+")) == NULL ){
        perror("ERROR: Cannot read groups from file\n");
        exit(EXIT_FAILURE);
    }

    while(fgets(line, sizeof(line), fp)){
        line[strlen(line)-1] = '\0';  printf("line -> %s\n", line);
        strcpy(keepline, line);

        name = strtok(line, "|");
        if(name == NULL)
            break;
        if(strcmp(name, clientName) == 0){
            name = strtok(NULL, ":");  printf("name -> %s\n", name);
    
            strcat(clientMessage, MAG"(");
            strcat(clientMessage, name);
            strcat(clientMessage, "): "RESET);

            if((strcmp(name, chatee) == 0) || (strcmp(chatee, "$group") == 0)){
                name = strtok(NULL, "");  printf("message -> %s\n", name);
                if(name != NULL){
                    strcat(clientMessage, name);

                    printf("final -> %s\n", clientMessage);
                    if(socketArray == NULL){
                        send(socket, clientMessage, 2024, 0);
                    }else{
                        for(int i=0; i<10; i++){
                            send(socketArray[i], clientMessage, 2024, 0);
                        }
                    }
                }
            }else{
                fprintf(fp_temp, "%s\n", keepline);
            }
        }else{
            fprintf(fp_temp, "%s\n", keepline);
        }
        memset(clientMessage, 0, 2024);
    }
    fclose(fp);
    fclose(fp_temp);
    remove("savedMessages.list");
    rename("tempFile", "savedMessages.list");

    sem_post(&semaphore);

}
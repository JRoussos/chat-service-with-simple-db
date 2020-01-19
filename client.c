/** Chat Service Project
 * 
 * Author: John Roussos
 * License: MIT
 * 
 */ 

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

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


#define MAX_SIZE 1024
#define PORT 8080

char name[10], pass[10];

void *rThread_func( void *sock_desc );
void *sThread_func( void *sock_desc );
int welcomeMenu(char infoText[]);
int auth(int action, void *sock_desc);
void mainMenu(void *sock_desc);
int chatMenu(void *sock_desc);
void createGroup(void *sock_desc);
void manageGroup(void *sock_desc);
char *checkBeforeSend(char message[]);
void addContactOrGroup(void *sock_desc);

int main(int argc , char *argv[]){
    int sock_desc;
    struct sockaddr_in serv_addr;
    pthread_t read_thread, send_thread;
    char sbuff[MAX_SIZE],rbuff[MAX_SIZE];
    char ipAddress[15];

    //resize window to 125x30 https://invisible-island.net/xterm/ctlseqs/ctlseqs.html (Window manipulation)
    printf("\e[8;30;119t");

    if(argc==1){
      //no command line arguments found
      strcpy(ipAddress, "127.0.0.1");
    }else{ 
        //argumnets found
        strcpy(ipAddress, argv[1]);
    }

    int action = welcomeMenu("");

    if((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Failed creating socket\n");

    bzero((char *) &serv_addr, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ipAddress);
    serv_addr.sin_port = htons( PORT );

    if (connect(sock_desc, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Failed to connect to server\n");
        return -1;
    }

    int respond = auth(action, (void *) &sock_desc);
    // printf("%d\n",respond);
    while(respond == 0 || respond == 2){
      if(action == 49 && respond == 0){
        action = welcomeMenu("This username is taken. Try a difderent one.");
        respond = auth(action, (void *) &sock_desc);
      }else if(action == 50 && respond == 2){
        action = welcomeMenu("You are already logged in.");
        respond = auth(action, (void *) &sock_desc);
      }else if(action == 50 && respond == 0){
        action = welcomeMenu("The username or the password you entered is wrong.");
        respond = auth(action, (void *) &sock_desc);
      }
    }

    system("clear");
    printf("Connected successfully as %s\n", name);

    mainMenu((void *) &sock_desc);

      if (pthread_create(&read_thread, NULL, rThread_func, (void *) &sock_desc) < 0 ){
          perror("ERROR: Thread Creation\n");
          exit(EXIT_FAILURE);
      }
      if (pthread_create(&send_thread, NULL, sThread_func, (void *) &sock_desc) < 0 ){
          perror("ERROR: Thread Creation\n");
          exit(EXIT_FAILURE);
      }
      if (pthread_join(read_thread, NULL) < 0 ){
          perror("ERROR: Thread Join\n");
          exit(EXIT_FAILURE);
      }
      if (pthread_join(send_thread, NULL) < 0 ){
          perror("ERROR: Thread Join\n");
          exit(EXIT_FAILURE);
      }
    
    close(sock_desc);
    return 0;

}

void *rThread_func( void *sock_desc ){
  char rbuff[MAX_SIZE];

  while(1){
    bzero(rbuff,MAX_SIZE);
    if(recv( (*(int *) sock_desc), rbuff, MAX_SIZE, 0)==0){
      printf("ERROR: Connection Lost\n");
      exit(EXIT_FAILURE);
    }else{
      printf("\r%s\n", rbuff);
      printf("\r%s", "> ");
      fflush(stdout);  
    }
  }
}

void *sThread_func( void *sock_desc ){
  char sbuff[MAX_SIZE];
  puts(CYN "You can type 'exit' at any point to leave the chat.\n" RESET);
  
  while(1){
    printf("\r%s", "> ");
    fflush(stdout);
    fgets(sbuff, MAX_SIZE, stdin);
    
    // if(strcmp(sbuff, "\n") == 0)
    //   continue;

    if(strcmp(sbuff, "exit\n") == 0){
      printf("\rBye :)\n\n");
      exit(EXIT_SUCCESS);
    }
    send((*(int *) sock_desc), checkBeforeSend(sbuff), strlen(sbuff), 0);
  }
}

int welcomeMenu(char infoText[]){
  char answer;
  fflush(stdin);
  system("clear");
  
  puts("                       _                                _           _    _                   _             _   ");
  puts("       __      __ ___ | |  ___  ___   _ __ ___    ___  | |_  ___   | |_ | |__    ___    ___ | |__    __ _ | |_ ");
  puts("       \\ \\ /\\ / // _ \\| | / __|/ _ \\ | '_ ` _ \\  / _ \\ | __|/ _ \\  | __|| '_ \\  / _ \\  / __|| '_ \\  / _` || __|");
  puts("        \\ V  V /|  __/| || (__| (_) || | | | | ||  __/ | |_| (_) | | |_ | | | ||  __/ | (__ | | | || (_| || |_ ");
  puts("         \\_/\\_/  \\___||_| \\___|\\___/ |_| |_| |_| \\___|  \\__|\\___/   \\__||_| |_| \\___|  \\___||_| |_| \\__,_| \\__|");

  printf("\n");
  printf("%s\n", infoText);
  printf("1. Sign Up\n2. Log In\n");
  printf("Select action: ");
  
  answer = fgetc(stdin);
  // printf("%d\n", (int)answer);
  while(1){
    if(answer == '1' || answer == '2'){
      break;
    }else if(answer == '\n'){
      answer = fgetc(stdin);
    }else{
      printf("Please enter a valid answer(1/2): ");
      answer = fgetc(stdin);
    }    
  }

  while(1){
    printf("Enter name: ");
    scanf("%s", name);

    printf("Enter password: ");
    scanf("%s", pass);

    if(strlen(name)>10 || strlen(pass)>10)
      printf("The name and the password should not exceeds 10 characters each\n\n");
    else
      break;
  }

  return (int)answer;
}

int auth(int action, void *sock_desc){
  char auth[40];
  int respond;
  
  if(action == 49)
    strcpy(auth, "sig|");
  else if(action == 50)
    strcpy(auth, "log|");
  strcat(auth, name);
  strcat(auth, "|");
  strcat(auth, pass);

  send( (*(int *)sock_desc), auth, strlen(auth), 0) ;

  recv( (*(int *) sock_desc), &respond, sizeof(int), 0 );
  return respond;
}

void mainMenu(void *sock_desc){
  char answer;
  while(answer != '5'){
    printf("1.Chat\t2.Add Friend or Group\t3.Create Group\t4.Delete Group\t5.Exit\n");
    printf("Select action: ");

    answer = fgetc(stdin);
    // printf("%d\n", (int)answer);
    while(1){
      if(answer == '1' || answer == '2' || answer =='3' || answer == '4' || answer == '5'){
        break;
      }else if(answer == '\n'){
        answer = fgetc(stdin);
      }else{
        printf("Please enter a valid answer(1-5): ");
        answer = fgetc(stdin);
      }    
    }

    switch (answer){
      case '1'://Chat
        //0.no contacts no chating
        //1.chating is available
        if(chatMenu(sock_desc))
          return;
        else{
          sleep(2);
          break;
        }
      case '2'://Add Friend/Group
        addContactOrGroup(sock_desc);
        break;
      case '3'://Create Group
        createGroup(sock_desc);
        break;
      case '4'://Delete Group
        manageGroup(sock_desc);
        break;
      case '5':
        exit(EXIT_SUCCESS);
      default:
        printf("Under Construction..\n");
        exit(0);
    }
    system("clear");
  }
}

int chatMenu(void *sock_desc){
  char *line, respond[60], answer, sendable[30];
  bzero(respond, 60);

  printf("1.Chat with a contact\n2.Chat in a group\n");
  printf("Answer: ");
  answer = fgetc(stdin);
  while(1){
    if(answer == '1' || answer == '2'){
      break;
    }else if(answer == '\n'){
      answer = fgetc(stdin);
    }else{
      printf("Please enter a valid answer(1/2): ");
      answer = fgetc(stdin);
    }    
  }

  if(answer == '2'){//group
    char group[10];
    //sending request for receiving the list of the users contacts
    send( (*(int *)sock_desc), "$groups:", 9, 0);
    if(recv( (*(int *) sock_desc), respond, 60, 0)==0){
      printf("ERROR: Connection Lost\n");
      exit(EXIT_FAILURE);
    }
    if(strcmp(respond, "noGroups") == 0){
      printf("You have no groups at the moment to chat with.\n");
      return 0;
    }else{
      //users list
      line = strtok(respond, "|");
      while(line != NULL){
        printf("%s\n", line);
        line = strtok(NULL, "|");
      }

      printf("\nWitch of your groups do you want to chat on?\nWrite name: ");
      scanf("%s", group);

      //send name back to server
      strcpy(sendable, "$contactGroup:");
      strcat(sendable, group);

      send( (*(int *)sock_desc), sendable, strlen(sendable), 0);
      return 1;
    }

  }else if(answer == '1'){//contact
    char contact[10];
    //sending request for receiving the list of the users contacts
    send( (*(int *)sock_desc), "$contacts:", 11, 0);
    if(recv( (*(int *) sock_desc), respond, 60, 0)==0){
      printf("ERROR: Connection Lost\n");
      exit(EXIT_FAILURE);
    }
    if(strcmp(respond, "noContacts") == 0){
      printf("You have no contacts at the moment to chat with.\n");
      return 0;
    }else{
      //users list
      line = strtok(respond, "|");
      while(line != NULL){
        printf("%s\n", line);
        line = strtok(NULL, "|");
      }

      printf("\nWitch of your contacts do you want to chat with?\nWrite name: ");
      scanf("%s", contact);

      //send name back to server
      strcpy(sendable, "$contactChat:");
      strcat(sendable, contact);
      send( (*(int *)sock_desc), sendable, strlen(sendable), 0);
      return 1;
    }
  }
}

void createGroup(void *sock_desc){
  char groupName[20], sendableName[30];
  // system("clear");

  strcpy(sendableName, "$setGroup:");
  printf("Enter the name of the group: ");
  scanf("%s", groupName);
  strcat(sendableName, groupName);

  if(strlen(groupName)>10){
    printf("The name you choose is too big. Group coundn't be created.");
  }else{
    send((*(int *) sock_desc), sendableName, strlen(sendableName), 0);
    printf("Successfully created the group '%s'\n", groupName);
    sleep(2);
  }
}

void manageGroup(void *sock_desc){
  char *line, userGroups[60], groupToDelete[10];
  bzero(userGroups, 60);
  bzero(groupToDelete, 10);
  
  send((*(int *) sock_desc), "$manageGroup:", 13, 0);
  if(recv( (*(int *) sock_desc), userGroups, 60, 0) == 0){
    printf("ERROR: Connection Lost\n");
    exit(EXIT_FAILURE);
  }
  
  if(strcmp(userGroups, "nogroups") == 0){
    printf("You don't have any groups.\n");
    sleep(2);
    
  }else{
    printf("You have these groups: \n");
    line = strtok(userGroups, "|");
    while(line != NULL){
      printf("%s\n", line);
      line = strtok(NULL, "|");
    }
    printf("\nWrite the name of the group you want to delete: ");
    scanf("%s", groupToDelete);

    send((*(int *) sock_desc), groupToDelete, 10, 0);
  }
}

char *checkBeforeSend(char message[]){
    int i;
    for(i=0; i<strlen(message); i++){
        if(message[i] == '|' || message[i] == '$' || message[i] == ':'){
            for(int k=i; k<strlen(message); k++){
                message[k] = message[k+1];   
            }
        }
        if(message[i] == '\n'){
          message[i] = '\0';
        }
    }
    return message;
}

void addContactOrGroup(void *sock_desc){
  char answer, name[20], sendable[30];
  system("clear");
  printf("1.Add a new friend in your contacts list\n2.Add a new group chat to your list\n");
  printf("Answer: ");
  answer = fgetc(stdin);
  while(1){
    if(answer == '1' || answer == '2'){
      break;
    }else if(answer == '\n'){
      answer = fgetc(stdin);
    }else{
      printf("Please enter a valid answer(1/2): ");
      answer = fgetc(stdin);
    }    
  }
  
  if(answer == '1'){//add friend
    printf("Type the name of the user you want to add: ");
    scanf("%s", name);
    // fgets(name, 20, stdin);

    bzero(sendable, 30);
    strcpy(sendable, "$addFriend:");
    strcat(sendable, name);

    send( (*(int *)sock_desc), sendable, strlen(sendable), 0);
    
    recv( (*(int *) sock_desc), sendable, 30, 0);
    if(strcmp(sendable, "ok") == 0){
      printf("You have successfully added the user '%s' to your contacts list\n", name);
      sleep(2);
    }else{
      printf("The username '%s' does not exists\n", name);
      sleep(2);
    }
  }else{//add group
    printf("Type the name of the group you want to add: ");
    scanf("%s", name);

    strcpy(sendable, "$addGroup:");
    strcat(sendable, name);

    send( (*(int *)sock_desc), sendable, strlen(sendable), 0);
    
    recv( (*(int *) sock_desc), sendable, 30, 0);
    if(strcmp(sendable, "ok") == 0){
      printf("You have successfully added the group '%s' to your list\n", name);
      sleep(2);
    }else{
      printf("The group '%s' does not exists\n", name);
      sleep(2);
    }
  }
}
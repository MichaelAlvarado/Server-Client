#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/un.h>
#include <signal.h>
#include <stdarg.h>

// Prototypes for internal functions and utilities
void error(const char *fmt, ...);
int runClient(char *clientName, int numMessages, char **messages);
int runServer();
void serverReady(int signal);

bool serverIsReady = false;

// Prototypes for functions to be implemented by students
void server();
void client(char *clientName, int numMessages, char *messages[]);

void verror(const char *fmt, va_list argp)
{
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n");
}

void error(const char *fmt, ...)
{
    fprintf(stderr, "ERROR FOUND>>>");
    va_list argp;
    va_start(argp, fmt);
    verror(fmt, argp);
    va_end(argp);
    exit(1);
}

int runServer(int port) {
    int forkPID = fork();
    if (forkPID < 0)
        error("ERROR forking server");
    else if (forkPID == 0) {
        server();
        exit(0);
    } else {
        fprintf(stderr, "Main: Server(%d) launched...\n", forkPID);
    }
    return forkPID;
}

int runClient(char *clientName, int numMessages, char **messages) {
    fflush(stdout);
    fprintf(stderr, "Launching client %s...\n", clientName);
    int forkPID = fork();
    if (forkPID < 0)
        error("ERROR forking client %s", clientName);
    else if (forkPID == 0) {
        client(clientName, numMessages, messages);
        exit(0);
    }
    return forkPID;
}

void serverReady(int signal) {
    serverIsReady = true;
}

#define NUM_CLIENTS 2
#define MAX_MESSAGES 5
#define MAX_CLIENT_NAME_LENGTH 10

struct client {
    char name[MAX_CLIENT_NAME_LENGTH];
    int num_messages;
    char *messages[MAX_MESSAGES];
    int PID;
};

// Modify these to implement different scenarios
struct client clients[] = {
        {"Uno", 3, {"Mensaje 1-1", "Mensaje 1-2", "Mensaje 1-3"}},
        {"Dos", 3, {"Mensaje 2-1", "Mensaje 2-2", "Mensaje 2-3"}}
};

int main() {
    signal(SIGUSR1, serverReady);

    int serverPID = runServer(getpid());
    while(!serverIsReady) {
        sleep(1);
    }
    fprintf(stderr, "Main: Server(%d) signaled ready to receive messages\n", serverPID);

    for (int client = 0 ; client < NUM_CLIENTS ; client++) {
        clients[client].PID = runClient(clients[client].name, clients[client].num_messages,
                                        clients[client].messages);
    }
    
    for (int client = 0 ; client < NUM_CLIENTS ; client++) {
        int clientStatus;
        fprintf(stderr, "Main: Waiting for client %s(%d) to complete...\n", clients[client].name, clients[client].PID);
        waitpid(clients[client].PID, &clientStatus, 0);
        fprintf(stderr, "Main: Client %s(%d) terminated with status: %d\n",
               clients[client].name, clients[client].PID, clientStatus);
    }

    fprintf(stderr, "Main: Killing server (%d)\n", serverPID);
    kill(serverPID, SIGINT);
    int statusServer;
    pid_t wait_result = waitpid(serverPID, &statusServer, 0);
    fprintf(stderr, "Main: Server(%d) terminated with status: %d\n", serverPID, statusServer);
    return 0;
}



//*********************************************************************************
//**************************** EDIT FROM HERE *************************************
//#you can create the global variables and functions that you consider necessary***
//*********************************************************************************

#define PORT_NUMBER 44142

bool serverShutdown = false;

//Open a socket and return its number
int openSocket(){
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0){
        error("ERROR opening socket");
    }
    return socketfd;
}

//take the 'string'(1 parameter) given and copy it in reverse on the 'reverse'(2 parameter)
void reverseString(char *string, char *reverse){
    int i, j, k;
    for(i = 0; string[i] != '\0'; i++);
    {
        k = i-1;
    }
    for(j = 0; j <= i-1; j++)
    {
        reverse[j] = string[k];
        k--;
    }
}

void shutdownServer(int signal) {
 //Indicate that the server has to shutdown
 fprintf(stderr, "The Server will Shutdown\n");
 serverShutdown = true;
 //Wait for the children to finish
 pid_t wpid;
 int status = 0;
 while ((wpid = wait(&status)) > 0);
 //Exit
 exit(0);
}

void client(char *clientName, int numMessages, char *messages[])
{
    int socket, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    //Open the socket
    socket = openSocket();
    //Connect to the server
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"Client ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(PORT_NUMBER);
    
    if (connect(socket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("Client ERROR connecting");
        
    //For each message, write to the server and read the response
    for(int m = 0; m < numMessages; m++){
        strcpy(buffer, messages[m]);
        //strcpy(buffer, "Test Message");
        n = write(socket, buffer, strlen(buffer));
        if (n < 0) error("Client ERROR writing to socket");
        n = read(socket, buffer, 255);
        if (n < 0) error("Client ERROR reading from socket");
        printf("%s : %d : %s\n", clientName, getpid(), buffer);
        bzero(buffer,256);
    }
    //Accept connection and create a child proccess to read and write 

    //fflush(stdout);
    sleep(5);
    //Close socket
    close(socket);
}

void server()
{
    int socket, newsocket, n;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[256];
    
    //Handle SIGINT so the server stops when the main process kills it
    signal (SIGINT, shutdownServer);
    
    //Open the socket
    socket = openSocket();
    
    //Set Address of server
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_NUMBER);
   
    //Bind the socket
    if(bind(socket , (struct sockaddr *) &serv_addr, 
    sizeof(serv_addr)) < 0){
        error("Server ERROR on binding");
    }
    
    //Listen the socket to accept
    if(listen(socket, 5) < 0){error("Server ERROR not success listen");}
    printf("Server Listening\n");
    
    bzero(buffer, 256);

    //Signal server is ready
    kill(0, SIGUSR1);

    //Accept connection and create a child proccess to read and write
    while(!serverShutdown){
        int clilen = sizeof(cli_addr);
        printf("Server Accepting\n");
        newsocket = accept(socket, (struct sockaddr *) &cli_addr, &clilen);
        if (newsocket < 0) error("Server ERROR on accept");
        printf("Server Accepted Client\n");

        //Create Child Server proccess
        int childID = fork();
        if (childID < 0)
        error("ERROR forking server");
       else if (childID == 0) 
       {
            //Child Server here
            int i = 0;
            while(i<3){
            n = read(newsocket, buffer, 255);
            if(n < 0) error("Server ERROR reading from socket");
        
            char message[256];
            reverseString(buffer, message); //Reverse the received message
            n = write(newsocket, message, strlen(message));
            if(n < 0) error("Server ERROR writing to socket");
            i++;
            printf("Server : %d : %s\n", getpid(), buffer); //expected output 
            }
        close(newsocket);
        close(socket);
        exit(0);
        } 
    }
    //Close socket
    close(newsocket);
    close(socket);
}



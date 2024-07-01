#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MB_SIZE 1024

#ifndef SMTP_PORT
#define SMTP_PORT 7000
#endif

typedef struct { 
  char * name;
  char * address;
  int port;
  struct sockaddr_in core;
  int fd;
} SMTPServer;

SMTPServer * openSMTPServer(char * name, char * address) {
  SMTPServer * server = (SMTPServer *) malloc(sizeof(SMTPServer));

  struct sockaddr_in serverCore;
  serverCore.sin_family = AF_INET;
  serverCore.sin_port = htons(SMTP_PORT);
  serverCore.sin_addr.s_addr = inet_addr(address);

  server->name = &name[0];
  server->address = &address[0];
  server->port = SMTP_PORT;
  server->core = serverCore;
  server->fd = socket(AF_INET, SOCK_STREAM, 0);

  int isBinded = bind(server->fd, (struct sockaddr *) &(server->core), sizeof(server->core));

  if (isBinded != 0) {
    fprintf(stderr, "ERROR: the port: %d is already open.\n", SMTP_PORT);
    exit(-1);
  }

  listen(server->fd, 0);

  printf("> %s is listening on port: %d.\n", server->name, server->port);

  return server;
}

typedef struct {
  int fd;
  struct sockaddr_in core;
} SMTPClient;

void SMTPSendResponse(SMTPClient *client, int code, char * message, int messageSize) {
  char newMessage[messageSize + 6];
  sprintf(newMessage, "%d %s\n", code, message);
  send(client->fd, newMessage, sizeof(newMessage), MSG_OOB);
}

#define SMTP_SEND_RESPONSE(client, code, message) SMTPSendResponse(client, code, message, strlen(message))

#define SMTP_CLIENT_DATA_PARSER(token, code, message, runable) \
  if (strstr(clientDataBuffer, token) != NULL) { \
    SMTP_SEND_RESPONSE(&client, code, message); \
    runable; \
    memset(clientDataBuffer, '\0', MB_SIZE); \
    continue; \
  }

typedef struct {
  char * hello;
  char * from;
  char * to;
  char * data;
} Email; 

int main(void) {
  SMTPServer * server = openSMTPServer("F-MAIL", "127.0.0.1");
  SMTPClient client;

  while (true) {
    int clientCoreSize = sizeof(client.core);
    client.fd = accept(server->fd, (struct sockaddr *) &client.core, (socklen_t *) &clientCoreSize);

    char serverDataBuffer[MB_SIZE];
    memset(serverDataBuffer, '\0', MB_SIZE);

    sprintf(serverDataBuffer, "%s, SMTP is all ears.", server->name);
    SMTP_SEND_RESPONSE(&client, 220, serverDataBuffer);

    char clientDataBuffer[MB_SIZE];
    memset(clientDataBuffer, '\0', MB_SIZE);

    Email * email = malloc(sizeof(Email));
    email->data = calloc(sizeof(char), 1);

    while (true) {
      int message_size = recv(client.fd, clientDataBuffer, MB_SIZE, 0);
      
      if (message_size == 0) {
        close(client.fd);
        break;
      }

      char message[MB_SIZE];
      sprintf(message, "%s, nice to meet you", server->name);
      SMTP_CLIENT_DATA_PARSER("HELO ", 250, message, {
        char * hello = strtok(clientDataBuffer, "HELO ");
        hello[strlen(hello) - 1] = '\0';

        email->hello = malloc(strlen(hello) - 1);
        strcpy(email->hello, hello);
      });

      SMTP_CLIENT_DATA_PARSER("MAIL FROM: ", 250, "Ok", {
        char * emailFrom = strtok(clientDataBuffer, "MAIL FROM: ");
        emailFrom[strlen(emailFrom) - 1] = '\0';

        email->from = malloc(strlen(emailFrom) - 1);
        strcpy(email->from, emailFrom);
      });

      SMTP_CLIENT_DATA_PARSER("RCPT TO: ", 250, "Ok", {
        char * emailTo = strtok(clientDataBuffer, "RCPT TO: ");
        emailTo[strlen(emailTo) - 1] = '\0';

        email->to = malloc(strlen(emailTo) - 1);
        strcpy(email->to, emailTo);
      });

      SMTP_CLIENT_DATA_PARSER("DATA", 354, "End Data wih <CR><LF>.<CR><LF>", {
        if (strlen(email->data) != 0) {
          free(email->data);
          email->data = calloc(sizeof(char), 1);
        }

        memset(clientDataBuffer, '\0', MB_SIZE);
        while (true) {
          recv(client.fd, clientDataBuffer, MB_SIZE, 0);

          if (strcmp("\r\n.\r\n", clientDataBuffer) == 0) {
              memset(clientDataBuffer, '\0', MB_SIZE);
              break;
          }
        
          strcat(email->data, clientDataBuffer);
          memset(clientDataBuffer, '\0', MB_SIZE);
        }
      });

      SMTP_CLIENT_DATA_PARSER("QUIT", 221, "Bye", {
        close(client.fd);
        break;
      });
    }

    printf("From: %s\n", email->from);
    printf("To: %s\n", email->to);
    printf("%s\n", email->data);
    free(email);
  }

  return 0;
}


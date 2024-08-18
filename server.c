#include <asm-generic/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include "map.h"


#define PORT (8000)
#define REQ_SIZE (1024)
#define RES_SIZE (8192)
#define RES_HEADER_SIZE (1024)
#define NOT_FOUND_PAGE ("./data/notfound.html")
#define BAD_REQUEST_PAGE ("./data/badrequest.html")
#define NOT_IMPLEMENTED_PAGE ("./data/notimplemented.html")

typedef enum request_types {
  GET,
  HEAD,
  NOT_IMPLEMENTED
} req_t;

void handle_request(int);
void attach_response_header(char *, int, int);
void create_response(char *, char *, int, req_t);

hashmap *routes;

/*
 * Attaches a response header based on the status code that the response should contain. 
 */
void attach_response_header(char *response, int status_code, int flen) {
  char header[RES_HEADER_SIZE] = "HTTP/1.1 ";
  char buf[80];
  char status_msg[75]; // Status code + space makes up 4 bytes + leave 1 byte for '/0'

  switch(status_code) {
    case 200:
      strcpy(status_msg, "OK");
      break;
    case 400:
      strcpy(status_msg, "Bad Request");
      break;
    case 404:
      strcpy(status_msg, "Not Found");
      break;
    case 501:
      strcpy(status_msg, "Not Implemented");
      break;
    default:
      break;
  }

  // Add status code
  sprintf(buf, "%d %s\r\n", status_code, status_msg);
  strcat(header, buf);
  // Add content type
  strcat(header, "Content-Type: text/html\r\n");
  // Add content length
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "Content-Length: %d\r\n", flen);
  strcat(header, buf);
  // Add connection info
  strcat(header, "Connection: closed\r\n");
  // Retrieve current time
  time_t raw_time;
  struct tm *curr_time;
  time(&raw_time);
  curr_time = localtime(&raw_time);
  memset(buf, 0, sizeof(buf));
  strftime(buf, 79, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", curr_time);
  strcat(header, buf);
  // Add server info
  strcat(header, "Server: localhost (Windows)\r\n");
  // Copy header into response
  strcat(header, "\r\n");
  strcpy(response, header);

  printf("%s", header);
}

void create_response(char *filename, char *response, int clientfd, req_t req_type) {
  FILE *file = fopen(filename, "r");
  int fsize;
  if (file != NULL) {
    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
  }
  if (file == NULL) {
    file = fopen(NOT_FOUND_PAGE, "r");
    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    attach_response_header(response, 404, fsize);
  }
  else if (strcmp(filename, BAD_REQUEST_PAGE) == 0) {
    attach_response_header(response, 400, fsize);
  }
  else if (req_type == NOT_IMPLEMENTED) {
    attach_response_header(response, 501, fsize);
  }
  else {
    attach_response_header(response, 200, fsize);
  }
  if (req_type == HEAD) {
    fclose(file);
    file = NULL;
    return;
  }
  char response_body[RES_SIZE - RES_HEADER_SIZE - 1];
  int i = 0;
  char c;
  fseek(file, 0, SEEK_SET);
  while ((c = fgetc(file)) != EOF) {
    response_body[i] = c;
    i++;
  }
  fclose(file);
  file = NULL;
  strcat(response, response_body);
}

void handle_request(int clientfd) {
  char request[REQ_SIZE] = { 0 };
  if (read(clientfd, request, REQ_SIZE - 1) == -1) {
    printf("an error occurred while reading request. errno: %d\n", errno);
    return;
  }
  request[REQ_SIZE - 1] = '\0';
  char *request_body = strchr(request, ' ');
  char request_type[request_body - request + 1];

  // Grab request type
  strncpy(request_type, request, request_body - request);
  request_type[request_body - request] = '\0';
  request_body = strchr(request_body, '/');
  char response[RES_SIZE] = { 0 };
  if (request_body == NULL) {
    create_response(BAD_REQUEST_PAGE, response, clientfd, GET);
    send(clientfd, response, strlen(response), 0);
    return;
  }
  
  /*
  200 OK
  400 Bad Request
  404 Not Found
  500 Internal Server Error
  */

  // Grab requested path
  char *nspace = strchr(request_body, ' '); // a pointer to the next space (to grab the requested path)
  char request_path[nspace - request_body + 1];
  strncpy(request_path, request_body, nspace - request_body);
  request_path[nspace - request_body] = '\0';

  if (strcmp(request_type, "GET") == 0) {
    // Look for the requested file
    char *filename = search(routes, request_path);
    create_response(filename, response, clientfd, GET);
    send(clientfd, response, strlen(response), 0);
  }
  else if (strcmp(request_type, "HEAD") == 0) {
    // Look for the requested file
    char *filename = search(routes, request_path);
    create_response(filename, response, clientfd, HEAD);
    send(clientfd, response, strlen(response), 0);
  }
  else {
    create_response(NOT_IMPLEMENTED_PAGE, response, clientfd, NOT_IMPLEMENTED);
    send(clientfd, response, strlen(response), 0);
  }
}

int main() {
  // Create route mappings

  routes = create_map();
  add_entry(routes, "/", "./data/example.html");
  add_entry(routes, "/anime", "./data/anime.html");
  add_entry(routes, "/maplestory", "./data/game.html");

  // Create Socket/Server Startup

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  // allows addr/port to be used so bind doesn't fail
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  struct sockaddr_in servaddr = {
    AF_INET,
    htons(PORT),
    0 // localhost
  };

  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
    printf("bind failed. errno: %d\n", errno);
    return -1;
  }
  listen(sockfd, 10);
  struct sockaddr caddr;
  socklen_t caddr_size;
  int clientfd;
  while(1) {
    clientfd = accept(sockfd, &caddr, &caddr_size);
    handle_request(clientfd);
    close(clientfd);
  }
  free_map(routes);
  close(sockfd);
  return 0;
}

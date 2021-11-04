#ifndef COMMON_H_  
#define COMMON_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define MAX_TOKEN_LENGTH 255
#define MAX_MUL_TOKEN 1240000
#define MAX_PORT_NUMBER 65535
#define BUFF_SIZE 65535

#define DELIMITER_SPACE " "
#define DELIMITER_SEMICOLON ";"
#define DELIMITER_COMMA ","

#define END_OF_LINE " \r\n.\r\n"
#define END_OF_LINE_LENGTH 6

#define OK "200"
#define CONTINUE "300"
#define ABORT "400"
#define RESPONSE_CODE_LENGTH 3

#define NUMBER_OF_COMMANDS 7

extern bool atexit_flag;

typedef enum {
    HELO,
    AUTH,
    LSF,
    EXEC,
    DOWNLOAD,
    SIZE, 
    UPLOAD
} COMMAND;

extern const char *commands[NUMBER_OF_COMMANDS];
extern const int commands_length[NUMBER_OF_COMMANDS];

extern void Close(int fd);
extern ssize_t Write(int fd, const void *buf, ssize_t count); 
extern ssize_t Read(int fd, void *buf, size_t count); 
extern int Socket(int domain, int type, int protocol);
extern void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

extern bool setAtexitFunction(void (*freeResources) (void));
extern void errUsr(const char *msg, bool fatal_error);
extern void errSys(const char *msg, bool fatal_error);
extern void Free(void *ptr);
extern void *Malloc(size_t size);
extern void *Realloc(void *ptr, size_t size);
extern unsigned long int generateToken(const char *passphrase);
extern void clearStdinBuffer();
extern char *Fgets(char *buffer, int size, FILE *stream);
extern int getFileSize(char * file_name);
extern void readAck(int sock_fd); 
extern void writeAck(int sock_fd); 

#endif
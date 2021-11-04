#include "common.h"

#ifndef	_STRING_H
#include <string.h>
#endif

#ifndef	_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

const char *commands[NUMBER_OF_COMMANDS] = { "HELO", "AUTH", "LSF", "EXEC", "DOWNLOAD", "SIZE", "UPLOAD" };
const int commands_length[NUMBER_OF_COMMANDS] = { 4, 4, 3, 4, 8, 4, 6 };

void Free(void *ptr)
{
    if (NULL == ptr) {
        fprintf(stderr, "[FREE ERROR] invalid ptr == NULL\n");
        exit(EXIT_FAILURE);
    }

    free(ptr);
}

int getFileSize(char * file_name)
{
    int value;
    struct stat st;

    if (-1 == (value = stat(file_name, &st))) {
        errSys("[STAT ERROR]", false);
    }

    return value == -1 ? value : (int)st.st_size;
}

void writeAck(int sock_fd) 
{
    char response[RESPONSE_CODE_LENGTH];
    while(strncmp(OK, response, Read(sock_fd, response, RESPONSE_CODE_LENGTH)));
}

void readAck(int sock_fd) 
{
    Write(sock_fd, OK, RESPONSE_CODE_LENGTH);
}


void Close(int fd)
{
    int value = close(fd);

    if (-1 == value) {
        errSys("[CLOSE ERROR]", false);
    }
}

ssize_t Write(int fd, const void *buf, ssize_t count) 
{
    if (0 == count) {
        fprintf(stderr, "[WRITE ERROR] invalid size == 0\n");
        exit(EXIT_FAILURE);
    }

    ssize_t value = write(fd, buf, count);

    if (-1 == value) {
        errSys("[WRITE ERROR]", false);
    } else if  (value < count) {
        errSys("[WRITE WARNING] Sono stati scritti meno byte del dovuto", false);
    }
    
   return value;
}

ssize_t Read(int fd, void *buf, size_t count) 
{
    if (0 == count) {
        fprintf(stderr, "[READ ERROR] invalid size == 0\n");
        exit(EXIT_FAILURE);
    }

    ssize_t value = read(fd, buf, count);

    if (-1 == value) {
        errSys("[READ ERROR]", false);
    } 

    return value;
}


int Socket(int domain, int type, int protocol)
{
    int fd = socket(domain, type, protocol);

    if (-1 == fd) {
        errSys("[SOCKET ERROR]", true);
    }

    return fd;
}

void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int value = connect(sockfd, addr, addrlen);

    if (-1 == value) {
        errSys("[CONNECT ERROR]", true);
    }
}

bool setAtexitFunction(void (*freeResources) (void))
{
    bool status = false;
    long max_atexit = sysconf(_SC_ATEXIT_MAX);

    if (0 < max_atexit) {
        int function_registration = atexit(freeResources);
        if (0 == function_registration) {
            status = true; 
        } 
    }

    if (!status) fprintf(stderr, "[ATEXIT ERROR] Non è stato possibile registrare la funzione di uscita... Eseguirò la funzione manualmente\n");
    return status;
}

void errUsr(const char *msg, bool fatal_error)
{
    fprintf(stderr, "[USER ERROR]: %s\n", msg);
    if (fatal_error) exit(EXIT_FAILURE);
}

void errSys(const char *msg, bool fatal_error) 
{ 
    perror(msg); 
    if (fatal_error) exit(EXIT_FAILURE); 
}

void *Realloc(void *ptr, size_t size)
{
    if (0 == size) {
        fprintf(stderr, "[REALLOC ERROR] invalid size == 0\n");
        exit(EXIT_FAILURE);
    }

    void *ptr_new = realloc(ptr, size);

    if (NULL == ptr_new) {
        errSys("[REALLOC ERROR]", true);
    }

    return ptr_new;
}

void *Malloc(size_t size)
{
    if (0 == size) {
        fprintf(stderr, "[MALLOC ERROR] invalid size == 0\n");
        exit(EXIT_FAILURE);
    }

    void *ptr = malloc(size);

    if (NULL == ptr) {
        errSys("[MALLOC ERROR]", true);
    }

    return ptr;
}


unsigned long int generateToken(const char *passphrase)
{
    unsigned long int token = 1;
    int passphraseLength = (int) strlen(passphrase);

    for (int i = 0; i < passphraseLength; i++)
        token += passphrase[i] * (token % MAX_MUL_TOKEN);

    token += passphraseLength;
    
    return token;
}

void clearStdinBuffer()
{
    int character;
    while ((character = getchar()) != '\n' && character != EOF) { }
}

char *Fgets(char *buffer, int size, FILE *stream)
{
    if (0 == size) {
        fprintf(stderr, "[FGETS ERROR] invalid size == 0\n");
        exit(EXIT_FAILURE);
    }

    char *ptr = fgets(buffer, size, stream);

    if (NULL == ptr) {
        if (feof(stream)) {
            fprintf(stderr, "[FGETS ERROR] end of file occurs while no characters have been read\n");
        } else if (EINTR == errno) {
            fprintf(stderr, "[FGETS ERROR] Interrupted system call (forse in seguito alla ricezione di un segnale), riseguo la funzione\n");
            return Fgets(buffer, size, stream);
        } else {
            errSys("[FGETS ERROR]", true);
        }

    } else {

        size_t ptr_len = strlen(ptr);
        if (10 == ptr[ptr_len - 1]) {
            ptr[ptr_len - 1] = '\0';
        } else {
            clearStdinBuffer();
        }
    }

    return ptr;
}
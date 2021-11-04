#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef	_STRING_H
#include <string.h>
#endif

#ifndef	_STDLIB_H
#include <stdlib.h>
#endif

#ifndef	_UNISTD_H
#include <unistd.h>
#endif

#ifndef	_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifndef	_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifndef _ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifndef	_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "../common.h"

#define DELIMITER_DOT "."

static unsigned long int T_s, T_c_i;

static char *serverIp = NULL;
static unsigned short serverPort;
static char *command = NULL;
static char *command_argument = NULL;

static int fd;

void setServerIpNumber(const char *tempServerIpNumber)
{
    char *part;
    unsigned short int value = 0;

    size_t ip_size = strlen(tempServerIpNumber);
    serverIp = (char*) Malloc(ip_size + 1);
    memcpy(serverIp, tempServerIpNumber, strlen(tempServerIpNumber));
    serverIp[ip_size] = 0;

    for(int i = 0; i < 4; i++) {
        if (i == 0) {
            part = strtok((char *) tempServerIpNumber, DELIMITER_DOT);
        } else {
            part = strtok(NULL, DELIMITER_DOT);
        }
        
        if (part) {
            value = atoi(part);
            if (value <= 255) {
                continue;
            }
        }
        errUsr("Formato dell'ip non valido", true);
    }
}

void setPortNumber(const int tempPortNumber)
{
    if (tempPortNumber > 0 && tempPortNumber <= MAX_PORT_NUMBER) {
        serverPort = tempPortNumber;
    } else {
        errUsr("Non è stato possibile acquisire un numero di porta valido", true);
    }
}

void setCommand(const char *cmd, const int num_args, const char **argv, int index)
{
    size_t cmd_length = strlen(cmd);
    size_t argument_length = 0;
    size_t total_arguments_length = 0;
 
    command = (char*) Malloc(cmd_length + 1);
    memcpy(command, cmd, cmd_length);
    command[cmd_length] = 0;

    while(index < num_args) {
        argument_length = strlen(argv[index]);
        command_argument = (char*) Realloc(command_argument, total_arguments_length + argument_length + 16);
        memcpy(&command_argument[total_arguments_length], argv[index], argument_length);
        total_arguments_length += argument_length + 1;
        command_argument[total_arguments_length - 1] = ' ';
        index += 1;
    }
    
    command_argument[total_arguments_length - 1] = 0;
}

void getArguments(const int num_args, const char **argv)
{
    int temp_port_number = 0;
    bool frist_condition, second_condition, third_condition;

    for (int i = 1; i < num_args; i++) {

        frist_condition = i + 1 < num_args && '-' != argv[i+1][0];
        second_condition = frist_condition && (i + 2 == num_args || '-' == argv[i+2][0]);
        third_condition = i + 2 < num_args && '-' != argv[i+1][0] && '-' != argv[i+2][0] && (i + 3 == num_args || '-' == argv[i+3][0]);

        if (!strcmp("-p", argv[i])) {
            if (frist_condition) {
                temp_port_number = atoi(argv[i+1]);
                setPortNumber(temp_port_number);
            } else {
                errUsr("L'opzione -p richiede un numero di porta", true);
            }
        } else if (!strcmp("-h", argv[i])) {
            if (frist_condition) {
                setServerIpNumber((char *) argv[i+1]);
            } else {
                errUsr("L'opzione -h richiede un indirizzo ip", true);
            }
        } else if (!command || !command_argument) {
            if (!strcmp("-e", argv[i])) {
                if (frist_condition) {
                    setCommand("EXEC", num_args, argv, i+1);
                } else {
                    errUsr("L'opzione -e richiede almeno un argomento (<cmd [args ...]>)", true);
                }
            } else if (!strcmp("-l", argv[i])) {
                if (second_condition) {
                    setCommand("LSF", num_args, argv, i+1);
                } else {
                    errUsr("L'opzione -l richiede un argomento (<path>)", true);
                }
            } else if (!strcmp("-d", argv[i])) {
                if (third_condition) {
                    if (!access(argv[i+1], F_OK)) {
                        setCommand("DOWNLOAD", num_args, argv, i+1);
                    } else {
                        errUsr("Al path specificato (<src-path>) non è presente un file", true);
                    }
                } else {
                    errUsr("L'opzione -d richiede due argomenti (<src-path> <dest-path>)", true);
                }
            } else if (!strcmp("-u", argv[i])) {
                if (third_condition) {
                    setCommand("UPLOAD", num_args, argv, i+1);
                } else {
                    errUsr("L'opzione -u richiede due argomenti (<src-path> <dest-path>)", true);
                }
            }
        }
    }

    if (!serverIp || 0 == serverPort || !command || !command_argument) {
        errUsr("Non hai inserito tutti gli argomenti necessari per eseguire il programma", true);
    }
}

void freeResources()
{
    if (serverIp) Free(serverIp);
    if (command) Free(command);
    if (command_argument) Free(command_argument);
    
    printf("Client terminato correttamente\n");
}

int authentication(char *buff, const int fd)
{    
    int status = -1;
    unsigned long int challenge;
    unsigned long int enc1, enc2;

    ssize_t length;

    if (commands_length[HELO] == Write(fd, commands[HELO], commands_length[HELO])) {
        if (RESPONSE_CODE_LENGTH == Read(fd, buff, RESPONSE_CODE_LENGTH)) {
            if (!strncmp(buff, CONTINUE, RESPONSE_CODE_LENGTH)) {
                memset(buff, false, RESPONSE_CODE_LENGTH);
                if (0 != (length = Read(fd, buff, MAX_TOKEN_LENGTH))) {
                    challenge = strtoul(buff, NULL, 10);
                    challenge = T_s ^ challenge;
                    enc1 = T_s ^ challenge ^ T_c_i;
                    enc2 = T_c_i ^ challenge;
                    memset(buff, false, length);
                    length = sprintf(buff, "%s %lu;%lu", commands[AUTH], enc1, enc2);
                    if (length == Write(fd, buff, length)) {
                        memset(buff, false, length);
                        if (RESPONSE_CODE_LENGTH == Read(fd, buff, RESPONSE_CODE_LENGTH)) {
                            if (!strncmp(buff, OK, RESPONSE_CODE_LENGTH)) {
                                status = 0; 
                            }
                        }
                    }
                }
            }
        }
    }

    memset(buff, false, RESPONSE_CODE_LENGTH);
    return status;
}

void protocol(const int sockfd)
{
    char buff[BUFF_SIZE] = {0};
    FILE *out;
    FILE *in;

    int bytes = 0;
    char * file_name;
    char * temp_file_name;
    char * temp_command;
    char extra_args[1024] = {0};

    if (0 == authentication(buff, sockfd)) {
        printf("Autenticato con successo\n");
        size_t file_size;
        size_t length;
        if (!strncmp(command, commands[UPLOAD], commands_length[UPLOAD])) {
            temp_file_name = strtok(command_argument, DELIMITER_SPACE);
            length = commands_length[SIZE] + 1 + strlen(temp_file_name) + 1;
            temp_command = Malloc(sizeof(char) * length);
            length = snprintf(temp_command, length, "%s %s", commands[SIZE], temp_file_name);

            printf("Invio il comando: %s\n", temp_command);

            if (length == (size_t) Write(sockfd, temp_command, length)) {
                Free(temp_command);
                if (RESPONSE_CODE_LENGTH == Read(sockfd, buff, RESPONSE_CODE_LENGTH)) {
                    if (!strncmp(buff, CONTINUE, RESPONSE_CODE_LENGTH)) {
                        length = Read(sockfd, buff+RESPONSE_CODE_LENGTH, BUFF_SIZE);
                        file_size = (size_t)atoi(buff+RESPONSE_CODE_LENGTH);
                        file_name = strtok(NULL, DELIMITER_SPACE);
                        sprintf(extra_args, "%s%lu", DELIMITER_COMMA, file_size);
                        memset(buff, false, RESPONSE_CODE_LENGTH + length);

                    } else if (!strncmp(buff, ABORT, RESPONSE_CODE_LENGTH)) {
                        Close(sockfd); 
                        errUsr("Comunicazione abortira", true); 
                    }
                } else { errUsr("Non ho ricevuto neanche 3 byte in seguito al comando SIZE", false); }
            } else { errUsr("Non sono riuscito a comunicare correttamente il comando SIZE (propedeutico al comando UPLOAD)", false); }
        } else if (!strncmp(command, commands[DOWNLOAD], commands_length[DOWNLOAD])) {
            file_name = strtok(command_argument, DELIMITER_SPACE);
            length = strlen(file_name) + 1;
            temp_file_name = Malloc(sizeof(char) * length);
            snprintf(temp_file_name, length, "%s", file_name);
            file_size = getFileSize(file_name);
            sprintf(command_argument, "%s%s%lu", strtok(NULL, DELIMITER_SPACE), DELIMITER_SEMICOLON, file_size);
        } else if (!strncmp(command, commands[EXEC], commands_length[EXEC])) {
            length = 0;
            for (size_t i = 0; command_argument[i]; i++) if (' ' == command_argument[i]) length++;
            strcpy(extra_args, command_argument);
            sprintf(command_argument, "%lu ", length);
        }

        length = sprintf(buff, "%s %s%s", command, command_argument, extra_args);
        printf("Invio il comando: %s\n", buff);

        if (length == (size_t) Write(sockfd, buff, length)) {
            memset(buff, false, length);
            if (!strncmp(command, commands[DOWNLOAD], commands_length[DOWNLOAD])) {
                if (NULL != (in = fopen(temp_file_name, "rb"))) {
                    while (file_size && 0 < (length = fread(buff, sizeof(char), BUFF_SIZE - 1, in))) {
                        file_size -= length;
                        if ((int)length != Write(sockfd, buff, length)) {
                            errUsr("Ho scritto meno byte nella socket rispetto a quanti ne ho letti dal file, interrompo l'esecuzione del comando", false);
                            break;
                        }
                    }
                    writeAck(sockfd);
                    fclose(in);
                } else { errSys("Non sono riuscito ad aprire il file al path desiderato\n[FOPEN ERROR]", false); }
                Free(temp_file_name);
            } else if (!strncmp(command, commands[UPLOAD], commands_length[UPLOAD])) {
                if (NULL != (out = fopen(file_name, "wb"))) {
                    while (file_size && 0 < (length = Read(sockfd, buff, BUFF_SIZE))) {
                        file_size -= length;
                        if ((int)length != (bytes = fwrite(buff, sizeof(char), length, out))) {
                            errUsr("Ho scritto meno byte rispetto a quanti ne ho letti... aborto la scrittura del file", false);
                            break;
                        }
                    }
                    readAck(sockfd);
                    fclose(out);
                } else { errSys("Non sono riuscito ad aprire il file al path desiderato\n[FOPEN ERROR]", false); }
            } else if (RESPONSE_CODE_LENGTH == Read(sockfd, buff, RESPONSE_CODE_LENGTH)) {
                if (!strncmp(buff, CONTINUE, RESPONSE_CODE_LENGTH)) {
                    memset(buff, false, RESPONSE_CODE_LENGTH);
                    length = 0;
                    while ((bytes = Read(sockfd, buff + length, BUFF_SIZE - length - 1))) {
                        length += bytes;
                        buff[length] = 0;

                        if (length == BUFF_SIZE - 1) {
                            printf("%s", buff);
                            length = 0;
                        }

                        if (NULL != strstr(buff, END_OF_LINE)) {
                            printf("%s", buff);
                            break;
                        }
                    }
                } else { errUsr("Il comando ha prodotto un errore", false); }
            } else { errUsr("Non ho ricevuto neanche 3 byte", false); }
        } else { errUsr("Non ho inviato correttamente il comando", false); }
    } else { errUsr("Autenticazione fallita", false); }

    Close(sockfd);
}

int main(int argc, const char **argv)
{
    bool atexit_flag = setAtexitFunction(freeResources);
    getArguments(argc, argv);

    char server_passphrase[255], client_i_passphrase[255];

    printf("Inserisci la passphrase inserita all'avvio del server: (verranno presi i primi 254 caratteri inseriti)\n");
    Fgets(server_passphrase, MAX_TOKEN_LENGTH, stdin);

    printf("Inserisci la tua passphrase: (verranno presi i primi 254 caratteri inseriti)\n");
    Fgets(client_i_passphrase, MAX_TOKEN_LENGTH, stdin);
    
    T_s = generateToken(server_passphrase);
    T_c_i = generateToken(client_i_passphrase);

    printf("\n\n|----------Resoconto dei parametri di esecuzione----------|\n|\n");
    printf("| server ip: %s\n", serverIp);
    printf("| server port: %hu\n", serverPort);
    printf("| comando: %s\targomento del comando: %s\n", command, command_argument);
    printf("|\n|---------------------------------------------------------|\n\n\n");

    uint16_t server_port = serverPort;
    struct sockaddr_in server_addr;
    socklen_t len_addr = sizeof(server_addr);
    memset(&server_addr, false, len_addr);

    fd = Socket(AF_INET, SOCK_STREAM, false);            

    server_addr.sin_addr.s_addr = inet_addr(serverIp);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    Connect(fd, (const struct sockaddr*) &server_addr, len_addr);
    protocol(fd);

    if (!atexit_flag) {
        freeResources();
    }

    return 0;
}

/* Avoid tipo incompleto non consentito (sizeof && sigaction) */
#define _XOPEN_SOURCE 700

#include "../common.h"

#ifndef	_STRING_H
#include <string.h>
#endif

#ifndef	_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifndef _ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifndef _ERRNO_H
#include <errno.h>
#endif


#ifndef	_UNISTD_H
#include <unistd.h>
#endif

#ifndef	_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>


#define MAX_PENDING_CONNECTION 64
#define MAX_THREADS_NUM 256
#define DEFAULT_THREADS_NUMBER 10
#define DEFAULT_PORT_NUMBER 8888
#define DELIMITER_EQUAL "="
#define READ "r"
#define WRITE "w"
#define MAX_ACCEPT 10 
 
static bool print_token = false;
static bool force_reloading = false;
static bool force_restart = false;
static unsigned short port_number = DEFAULT_PORT_NUMBER;
static unsigned short threads_number = DEFAULT_THREADS_NUMBER;
static unsigned short threads_number_temp;
static char * log_file_name = "/tmp/server.log";
static FILE * ptr_log = NULL;
static char * config_file_name;
static FILE * ptr_config = NULL;
static FILE * ptr_syslog = NULL;

static unsigned long int T_s;

typedef struct my_thread {
    pthread_t th;
    int state;
    int fd;
    char client_ip[16];
    char client_port[6];
    int timestamp;
    FILE * file;
    pthread_mutex_t mutex;
} my_thread;

static sem_t mutex;
static pthread_mutex_t mutex_log;

static const char *exec_commands[NUMBER_OF_COMMANDS] = { "copy", "remove", "printworkdir" };
static const char *unix_exec_commands[NUMBER_OF_COMMANDS] = { "cp", "rm", "pwd"};

void writeLog(my_thread * arguments, char * command)
{
    pthread_mutex_lock(&mutex_log);
    fprintf(ptr_log, "%lu;%s;%s;%s;%d\n", arguments->th, arguments->client_ip, arguments->client_port, command, arguments->timestamp);
    pthread_mutex_unlock(&mutex_log);
}

void setThreadsNumber(int temp_threads_number, bool via_config)
{
    if (0 < temp_threads_number && MAX_THREADS_NUM >= temp_threads_number && (!via_config || force_reloading || DEFAULT_THREADS_NUMBER == threads_number)) {
        threads_number = temp_threads_number;
    } else if (DEFAULT_THREADS_NUMBER == threads_number && via_config) { 
        errUsr("Non è stato possibile acquisire un numero di thread valido dal file di configurazione", false); 
    } else if (!via_config) {
        errUsr("Non è stato possibile acquisire un numero di thread valido da linea di comando", false); 
    }
}

void setPortNumber(int temp_port_number, bool via_config)
{
    if (0 < temp_port_number && MAX_PORT_NUMBER >= temp_port_number) {
        if  (!via_config || force_reloading || DEFAULT_PORT_NUMBER == port_number) {
            if (1024 > temp_port_number) {
                errUsr("I numeri di porta inferiori a 1024 sono considerati privilegiati e quindi necessitano dei permessi di root per poter essere utilizzate", false); 
            }
            port_number = temp_port_number;
        }
    } else if (via_config) {
        errUsr("Non è stato possibile acquisire un numero di porta valido dal file di configurazione", false); 
    } else {
        errUsr("Non è stato possibile acquisire un numero di porta valido", false); 
    }
}

void getParametersFromConfig(FILE * ptr_config)
{
    char * line = NULL;
    size_t length = 0;
    ssize_t size;

    char * ptr_parameter;
    char * ptr_value;
    int value = 0;

    while ((size = getline(&line, &length, ptr_config)) != -1) {
        ptr_parameter = strtok(line, DELIMITER_EQUAL);

        if (ptr_parameter) {
            ptr_value = strtok(NULL, DELIMITER_EQUAL);
            value = atoi(ptr_value);

            if (value > 0) {
                if (!strcmp(ptr_parameter, "PORT_NUMBER")) {
                    setPortNumber(value, true);
                } else if (!strcmp(ptr_parameter, "STDOUT_TOKEN")) {
                    print_token = true;
                } else if (!strcmp(ptr_parameter, "THREADS_NUMBER")) {
                    setThreadsNumber(value, true);
                }
            }
        }
    }
    if (line) Free(line);
}

void handlerSighup()
{
    rewind(ptr_config);
    if (!force_reloading) force_reloading = true;
    force_restart = true;
    getParametersFromConfig(ptr_config);
}

void createSignalHandler()
{
    struct sigaction sighup_action;
    memset(&sighup_action, 0, sizeof(sighup_action));

    if (ptr_config) {
        sighup_action.sa_handler = handlerSighup;
    } else {
        sighup_action.sa_handler = SIG_IGN;
    }

    if (sigaction(SIGHUP, &sighup_action, NULL) < 0) {
        errSys("Non è stato possibile creare un handler per il segnale SIGHUP\n[SIGACTION ERROR]", false);
    }
}


void getArguments(int num_args, const char *args[])
{
    int temp_port_number = 0;
    int temp_threads_number = 0;

    for (int i = 1; i < num_args; i++) {
        if (!strcmp("-s", args[i])) {
            print_token = true;
        } else if (!strcmp("-l", args[i])) {
            if (i + 1 < num_args && args[i+1][0] != '-') {
                if ((ptr_log = fopen(args[i+1], "w")) == NULL) {
                    errUsr("Non è stato possibile usare il file dei log al path specificato", false); 
                } else { 
                    log_file_name = (char *)args[i+1];
                    continue; 
                }
            }  
        } else if (!strcmp("-c", args[i])) {
            if (i + 1 < num_args) {
                if ((ptr_config = fopen(args[i+1], "r")) == NULL) {
                    errUsr("Non è stato possibile aprire il file delle configurazioni al path specificato", false); 
                    continue;
                } else { 
                    config_file_name = (char *)args[i+1];
                    getParametersFromConfig(ptr_config); 
                }
            } else { 
                errUsr("Non hai specificato il path in cui sono collocate le configurazioni... Non posso usarlo", false);  
            }         
        } else if (!strcmp("-p", args[i])) {
            if (i + 1 < num_args) {
                temp_port_number = atoi(args[i+1]);
                setPortNumber(temp_port_number, false);
            }
        } else if (!strcmp("-n", args[i])) {
            if (i + 1 < num_args) {
                temp_threads_number = atoi(args[i+1]);
                setThreadsNumber(temp_threads_number, false);
            }
        }
    }

    if (NULL == ptr_log && NULL == (ptr_log = fopen(log_file_name, "w"))) {
        errUsr("Non è stato possibile usare il file dei log al path di default '/tmp/server.log'", false); 
    }
}

void freeResources(void)
{
    if (ptr_config) fclose(ptr_config);

    if (ptr_log) {
        fclose(ptr_log);
        pthread_mutex_destroy(&mutex_log);
    }

    int sem_value_;
    sem_getvalue(&mutex, &sem_value_);

    if (0 != sem_value_) {
        sem_destroy(&mutex);
    }

    if (ptr_syslog) {
        fclose(ptr_syslog);
    }
}


void Listen(int sockfd, int backlog)
{
    int value = listen(sockfd, backlog);

    if (-1 == value) {
        errSys("[LISTEN ERROR]", true);
    }
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int value = bind(sockfd, addr, addrlen);

    if (-1 == value) {
        errSys("[BIND ERROR]", true);
    }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t addrlen)
{
    int value = accept(sockfd, addr, &addrlen);

    if (-1 == value) {
        if (EINTR == errno) {
            if (ptr_config) {
                fprintf(stderr, "[ACCEPT ERROR] Interrupted system call (forse in seguito alla ricezione di un segnale), riavvio il server!\n");
                value = -1; 
            } else {
                fprintf(stderr, "[ACCEPT ERROR] Interrupted system call (forse in seguito alla ricezione di un segnale), rieseguo la funzione\n");
                return Accept(sockfd, addr, addrlen);
            }
        } else {
            errSys("[ACCEPT ERROR]", true);
        }
    }

    return value;
}

int authentication(char *buff, int fd)
{
    int status = -1;    

    int length = 0;
    char * save_ptr;
    char * cmd, * enc1, * enc2;

    unsigned int seed;
    unsigned long int random;
    unsigned long int challenge;
    unsigned long int enc1_c = 0, enc2_c = 0;
    unsigned long int T_c_i, challenge_c;

    if (commands_length[HELO] == Read(fd, buff, sizeof(char) * commands_length[HELO])) {
        if (0 == strncmp(buff, commands[HELO], sizeof(char) * commands_length[HELO])) {
            memset(buff, false, sizeof(char) * commands_length[HELO]);

            if (RESPONSE_CODE_LENGTH == Write(fd, CONTINUE, sizeof(char) * RESPONSE_CODE_LENGTH)) {
                seed = (unsigned int) time(NULL);
                random = rand_r(&seed);
                challenge = T_s ^ random;

                char randomToken[MAX_TOKEN_LENGTH] = {0};
                snprintf(randomToken, MAX_TOKEN_LENGTH, "%lu", challenge);
                length = strnlen(randomToken, sizeof(char) * (MAX_TOKEN_LENGTH - true));
                
                if (length == Write(fd, randomToken, sizeof(char) * length)) {
                    if (commands_length[AUTH] + 4 <= Read(fd, buff, sizeof(char) * (commands_length[AUTH] + MAX_TOKEN_LENGTH + MAX_TOKEN_LENGTH))) {
                        cmd = strtok_r(buff, DELIMITER_SPACE, &save_ptr);

                        if (cmd && !strncmp(cmd, commands[AUTH], commands_length[AUTH])) {
                            enc1 = strtok_r(NULL, DELIMITER_SEMICOLON, &save_ptr);

                            if (enc1 && (enc1_c = strtoul(enc1, NULL, 10))) {
                                
                                enc2 = strtok_r(NULL, DELIMITER_SEMICOLON, &save_ptr);

                                if (enc2 && (enc2_c = strtoul(enc2, NULL, 10))) {
                                                                     
                                    T_c_i = T_s ^ challenge ^ enc1_c;
                                    challenge_c = T_c_i ^ enc2_c;    

                                    if (challenge_c == challenge) {
                                        status = 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    memset(buff, false, sizeof(char) * (commands_length[AUTH] + MAX_TOKEN_LENGTH + MAX_TOKEN_LENGTH));
    return status;
}

FILE *Popen(const char *command, const char *type)
{
    if (NULL == command) {
        fprintf(stderr, "[POPEN ERROR] invalid command\n");
        exit(EXIT_FAILURE);    
    }
    if (NULL == type || (strcmp(type, READ) && strcmp(type, WRITE))) {
        fprintf(stderr, "[POPEN ERROR] invalid type (r || w)\n");
        exit(EXIT_FAILURE);
    }

    FILE *pipe = popen(command, type);

    if (NULL == pipe) {
        errSys("[POPEN ERROR]", false);
    }

    return pipe;
}


int Pclose(FILE *stream)
{
    int status;

    status = pclose(stream);

    if (-1 == status) {
        errSys("[PCLOSE ERROR]", false);
    }

    return status;
}

int Pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
    int status;

    status = pthread_mutex_init(mutex, attr);

    if (0 != status) {
        errSys("[PTHREAD_MUTEX_INIT ERROR]", true);
    }

    return status;
}

int Pthread_mutex_lock(pthread_mutex_t *mutex)
{
    int status;

    status = pthread_mutex_lock(mutex);

    if (0 != status) {
        errSys("[PTHREAD_MUTEX_LOCK ERROR]", true);
    }

    return status;
}

int Pthread_mutex_unlock(pthread_mutex_t *mutex) 
{
    int status;

    status = pthread_mutex_unlock(mutex);

    if (0 != status) {
        errSys("[PTHREAD_MUTEX_UNLOCK ERROR]", true);
    }

    return status;
}

int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
    int status;

    status = pthread_create(thread, attr, start_routine, arg);

    if (0 != status) {
        errSys("[PTHREAD_CREATE ERROR]", true);
    }

    return status;
}


COMMAND findCommand(char *command)
{
    COMMAND result = HELO; 

    for (int i = (int)HELO; NULL != commands[i]; ++i, ++result) {
        if (!strcmp(command, commands[i])) {
            return result;
        } 
    }
        
    return -1;
}

int execCommand(char ** cmd) 
{
    int status = -1;

    for (int i = 0; NULL != exec_commands[i]; i++ ) {
        if (!strcmp(*cmd, exec_commands[i])) {
            *cmd = (char *)unix_exec_commands[i];
            status = 0;
            break;
        }
    }

    return status;
}

int executeCommand(char **command, char *command_argument, char *buff, my_thread *args)
{
    COMMAND cmd = findCommand(*command);
    my_thread * arguments = (my_thread*)args;

    printf("Esecuzione del comando seguente: %s %s\n", *command, command_argument);

    int file_length = 0;
    ssize_t temp;
    ssize_t length = 0;
    char * frist_arg, * second_arg, * save_ptr, * temp_ptr = NULL;
    int exit_status = -1;
    bool flag = false;

    FILE * io = NULL;

    if (SIZE == cmd) {
        if (-1 == (file_length = getFileSize(command_argument))) {
            errUsr("Non è stato possibile reperire la size del file al path specificato\n", false);
            if (RESPONSE_CODE_LENGTH != Write(arguments->fd, ABORT, RESPONSE_CODE_LENGTH)) {
                printf("Si è verificato un errore nell'abortire la comunicazione\n");
            }
        } else if (RESPONSE_CODE_LENGTH == Write(arguments->fd, CONTINUE, RESPONSE_CODE_LENGTH)) {
            length = sprintf(buff, "%d", file_length);

            if (length == Write(arguments->fd, buff, length)) {
                if (0 != (length = Read(arguments->fd, buff, BUFF_SIZE - 1))) {
                    printf("Esecuzione del comando seguente: %s\n", buff);
                    if (!strncmp(buff, commands[UPLOAD], commands_length[UPLOAD])) {

                        if (ptr_log) {
                            writeLog(arguments, *command);
                        }

                        Free(*command);
                        *command = (char *)commands[UPLOAD];
                        frist_arg = strtok_r(&buff[commands_length[UPLOAD]+1], DELIMITER_COMMA, &save_ptr);
                        second_arg = strtok_r(NULL, DELIMITER_COMMA, &save_ptr);
                        file_length = atoi(second_arg);

                        if (NULL != (io = fopen(frist_arg, "rb"))) {
                            while (file_length) {
                                if (file_length > (BUFF_SIZE - 1)) {
                                    temp = fread(buff, sizeof(char), BUFF_SIZE - 1, io);
                                } else {
                                    temp = fread(buff, sizeof(char), file_length, io);
                                }

                                if (temp != Write(arguments->fd, buff, temp)) {
                                    printf("Si è verificato un errore durante la trasmissione del file\n");
                                    break;
                                }                                
                                file_length -= temp;
                            }
                            writeAck(arguments->fd);
                        }
                    } else {
                        printf("Non mi è arrivato il comando upload in seguito al comando SIZE\n");
                    }
                }
            } else {
                printf("La size del file è stata inviata parzialmente\n");
            }
        }
    } else if (DOWNLOAD == cmd) {
        frist_arg = strtok_r(command_argument, DELIMITER_SEMICOLON, &save_ptr);
        file_length = atoi(strtok_r(NULL, DELIMITER_SEMICOLON, &save_ptr));

        if (NULL != (io = fopen(frist_arg, "wb"))) {
            while (file_length) {
                if (file_length > (BUFF_SIZE - 1)) {
                    temp = Read(arguments->fd, buff, BUFF_SIZE - 1);
                } else {
                    temp = Read(arguments->fd, buff, file_length);
                }

                if (temp != (ssize_t) fwrite(buff, sizeof(char), temp, io)) {
                    printf("Si è verificato un errore durante la scrittura del file\n");
                    break;
                }
                
                file_length -= temp;
            }
            readAck(arguments->fd);
        }
    } else if (EXEC == cmd || LSF == cmd) {
        frist_arg = strtok_r(command_argument, DELIMITER_SPACE, &save_ptr);

        if (EXEC == cmd) {
            second_arg = strtok_r(NULL, DELIMITER_SPACE, &save_ptr);
            execCommand(&second_arg);
            sprintf(command_argument, "%s %s", second_arg, save_ptr);
        } else {
            second_arg = calloc(sizeof(char), strlen(frist_arg) + 30);
            sprintf(second_arg, "ls -l %s | awk '{print $5, $9}'", frist_arg);
            command_argument = temp_ptr = strdup(second_arg);
            Free(second_arg);
            flag = true;
        }

        io = Popen(command_argument, READ); 

        if (temp_ptr) Free(temp_ptr);

        Write(arguments->fd, CONTINUE, RESPONSE_CODE_LENGTH);

        if (!feof(io)) {
            while(fgets(buff, BUFF_SIZE - 1, io)) {
                length = strlen(buff);

                if (flag) {
                    sprintf(&buff[length - 1], "\r\n");
                    length += 1;
                }

                printf("Invio -> %s", buff);
                Write(arguments->fd, buff, length);
            }
        }
        
        exit_status = Pclose(io);

        printf("Terminazione comando con codice: %d\n", exit_status);

       
        if (0 != exit_status) Write(arguments->fd, buff, sprintf(buff, "%d \r\n.\r\n", exit_status));
        else Write(arguments->fd, END_OF_LINE, END_OF_LINE_LENGTH);        
    }

    if (-1 == exit_status && io) fclose(io);

    return length;
}

void * protocol(void * args)
{
    my_thread * arguments = (my_thread*)args;

    while (true) {

        Pthread_mutex_lock(&arguments->mutex);

        if (-1 != arguments->fd) {

            char buff[BUFF_SIZE] = {0};
            
            int status = -1;

            char * save_ptr;
            char * temp_command;
            char * command, * command_argument;
            int inital_length;
            ssize_t length;

            if (-1 != (status = authentication(buff, arguments->fd))) {
                if (RESPONSE_CODE_LENGTH == Write(arguments->fd, OK, RESPONSE_CODE_LENGTH)) {
                    if (0 != (inital_length = Read(arguments->fd, buff, sizeof(buff) - 1))) {
                        temp_command = strtok_r(buff, DELIMITER_SPACE, &save_ptr);

                        if (temp_command) {
                            length = strnlen(temp_command, commands_length[DOWNLOAD]);
                            command = Malloc(sizeof(char) * (length + 1));
                            memcpy(command, temp_command, length);
                            command[length] = 0;

                            if (save_ptr) {
                                length = strlen(save_ptr);
                                command_argument = Malloc(sizeof(char) * (length + 1));
                                memcpy(command_argument, save_ptr, sizeof(char) * length);
                                command_argument[length] = 0;
                                memset(buff, false, inital_length);

                                executeCommand(&command, command_argument, buff, arguments);
                                
                                if (ptr_log) {
                                    writeLog(arguments, command);
                                }
                            }

                            if (command && command != (char *)commands[UPLOAD]) {
                                Free(command);
                            }
                            if (command_argument) {
                                Free(command_argument);
                            }
                        }   
                    }
                } else { printf("Si è verificato un errore nel comunicare la riuscita dell'auth al client\n"); }
            } else { 
                errUsr("Autenticazione fallita, aborto la comunicazione", false);
                if (RESPONSE_CODE_LENGTH != Write(arguments->fd, ABORT, RESPONSE_CODE_LENGTH)) {
                    printf("Si è verificato un errore nel comunicare la non riuscita dell'auth al client\n");
                }
            }

            shutdown(arguments->fd, SHUT_WR);
            Close(arguments->fd);
            printf("Terminazione del client con indirizzo ip %s\tporta %s\n\n", arguments->client_ip, arguments->client_port);
            arguments->fd = -1;
            arguments->state = 0;
            sem_post(&mutex);
        } else {
            break;
        }
    }

    return NULL;
}

int Dup2(int oldfd, int newfd) 
{
    int value = dup2(oldfd, newfd);

    if (-1 == value) {
        errSys("[DUP2 ERROR]", false);
    }
    return value;
}


int main(int argc, const char *argv[])
{ 
    bool atexit_flag = setAtexitFunction(freeResources);
    getArguments(argc, argv);

    char passphrase[MAX_TOKEN_LENGTH];

    printf("Inserisci la passphrase con cui generare il token T_s (verranno presi i primi 254 caratteri inseriti):\n");
    Fgets(passphrase, MAX_TOKEN_LENGTH, stdin);

    T_s = generateToken(passphrase);

    if (print_token) {
        printf("T_s generato %lu\n", T_s);
    }

    size_t max = MAX_ACCEPT;

    printf("\n\n|----------Resoconto dei parametri di esecuzione del server----------|\n|\n");
    printf("| stdout token: %d\n", print_token);
    printf("| porta TCP utilizzata: %d\n", port_number);
    printf("| numero di thread disponibili: %d\n", threads_number);
    printf("| numero di connessioni dopo le quali il server terminerà l'esecuzione %zu\n", max);
    if (ptr_log) printf("| path del file dei log: %s\n", log_file_name);
    if (ptr_config) printf("| path del file delle configurazioni: %s\n", config_file_name);
    printf("|\n|--------------------------------------------------------------------|\n\n\n");

    printf("Premere q per abortire l'esecuzione del server oppure premere un qualsiasi altro tasto per proseguire l'esecuzione del server in background con i parametri precedentemente menzionati (seguito dal tasto invio):");
    printf("\n** L'output che verrà generato dal processo daemon verrà ridirezionato nel file (syslogd)\n");
    char cont = getchar();
    pid_t pid;

    if ('q' != cont) {
        pid = fork();

        if (-1 == pid) {
            errSys("Non è stato possibile creare un processo figlio:\n[FORK ERROR]", true);
        }

        if (0 == pid) {
            printf("Daemon creato correttamente\n");

            if (-1 == setsid()) {
                errSys("Non è stato possibile creare una nuova sessione:\n[SETSID ERROR]", true);
            }
        
            createSignalHandler();
            Close(STDIN_FILENO);

            int dup_syslog;

            if (NULL == (ptr_syslog = fopen("syslogd", "w"))) {
                errSys("Non è stato possibile aprire il file (syslogd)", false); 
                Close(STDOUT_FILENO);
                Close(STDERR_FILENO);
            } else {
                if (-1 != (dup_syslog = fileno(ptr_syslog))) {
                    Dup2(dup_syslog, STDOUT_FILENO);
                    Dup2(dup_syslog, STDERR_FILENO);
                } else {
                    Close(STDOUT_FILENO);
                    Close(STDERR_FILENO);
                }   
            }

            if (ptr_log) {
                if (0 != pthread_mutex_init(&mutex_log, NULL)) {
                    errSys("Non sarà possibile usare il file log a causa:\n[PTHREAD_MUTEX_INIT ERROR]", false);
                    ptr_log = NULL;
                }
            }

            struct sockaddr_in server_addr, client_addr;
            
            uint16_t server_port;
            socklen_t len_addr;
            int listen_fd;  
            int last_port_number = -1;
            
            while (max) {

                if (-1 == sem_init(&mutex, 0, threads_number)) {
                    errSys("Non è stato possibile inizializzare il semaforo\n[SEM_INIT ERROR]", true);
                } 

                my_thread * threads = (my_thread *) Malloc(sizeof(my_thread) * threads_number);
                for (int i = 0; i < threads_number; i++) {
                    threads[i].state = 0;
                    Pthread_mutex_init(&threads[i].mutex, NULL);
                    Pthread_mutex_lock(&threads[i].mutex);
                    Pthread_create(&threads[i].th, NULL, protocol, &threads[i]);
                }

                if (last_port_number != port_number) {
                    last_port_number = port_number;
                    server_port = port_number;
                    
                    listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

                    int reuseaddr = 1;
                    if (-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int))) {
                        errSys("Non è stato possibile settare l'opzione per il socket (SO_REUSEADDR), questo potrebbe comportare un errore sulla Bind se il server in precedenza è stato terminato forzatamente\n[SETSOCKOPT ERROR]", false);
                    }

                    len_addr = sizeof(server_addr);
                    memset(&server_addr, 0, len_addr);
                    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(server_port);

                    Bind(listen_fd, (struct sockaddr *) &server_addr, len_addr);
                    Listen(listen_fd, MAX_PENDING_CONNECTION);
                }
                

                int conn_fd;
                int timestamp;

                threads_number_temp = threads_number;

                while (max) {
                    sem_wait(&mutex);

                    if (force_restart || -1 == (conn_fd = Accept(listen_fd, (struct sockaddr *) &client_addr, len_addr))) {
                        conn_fd = -1;
                        break;
                    }

                    timestamp = (int) time(NULL);
                    
                    for (int i = 0; i < threads_number; i++) {
                        if (0 == threads[i].state) {
                            threads[i].state = 1;
                            threads[i].fd = conn_fd;
                            strcpy(threads[i].client_ip, inet_ntoa(client_addr.sin_addr));
                            sprintf(threads[i].client_port, "%d", (int) ntohs(client_addr.sin_port));
                            threads[i].timestamp = timestamp;
                            Pthread_mutex_unlock(&threads[i].mutex);
                            max--;
                            printf("\nConnessioni rimaste: %lu\n", max);
                            break;
                        }
                    }
                } 

                for (int i = 0; i < threads_number_temp; i++) {
                    if (0 == threads[i].state) {
                        threads[i].fd = -1;
                    }
                    
                    Pthread_mutex_unlock(&threads[i].mutex);
                    printf("Attendo la terminazione del thread con id: %lu\n", threads[i].th);
                    pthread_join(threads[i].th, NULL);
                    pthread_mutex_destroy(&threads[i].mutex);
                }

                Free(threads);

                if (last_port_number != port_number) {
                    Close(listen_fd);
                }

                if (-1 != conn_fd) {
                    break;
                }
                
                sem_destroy(&mutex);
                force_restart = false;
                printf("Riavvio il server con i parametri presenti nel file config\n");
            }
            printf("Daemon terminato correttamente\n");
        }
    } 

    if (!atexit_flag) {
        freeResources();
    }

    if ('q' == cont || 0 < pid) {
        printf("Main process terminato correttamente\n");
    }

    exit(EXIT_SUCCESS);
}
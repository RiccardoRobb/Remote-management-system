#ifndef _WINDOWS_H
#include <windows.h>
#endif

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _STRING_H
#include <string.h>
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

#ifndef _ERRNO_H
#include <errno.h>
#endif

#ifndef _PROCESS_H
#include <process.h>
#endif

#ifndef _WINSOCK2_H
#include <winsock2.h>
#endif

#ifndef _WS2TCPIP_H
#include <ws2tcpip.h>
#endif

#ifdef _TIME_H
#include <time.h>
#endif

#ifndef _INTTYPES_H_
#include <inttypes.h> // per printare large_int aka. %lld
#endif

#ifndef _FUN_H
#include "./../functions.h"
#endif

// server args: -p = setTCPPort, -n = maxTh (default 10), -c = configurationPath, -s = printToken_s, -l = logPath (default /tmp/server.log)
DWORD token_S;
HANDLE sem;

typedef struct thread{
    BOOL state;
    DWORD index;
    HANDLE log;
    HANDLE handle;
    SOCKET sock;
    CHAR port[6];
    CHAR address[16];
    FILETIME timestamp;
} thread;

struct thread *thState;

INT authentication(SOCKET sock){
    CHAR msg[BUFF_SIZE] = {0};
    CHAR *ptr;
    CHAR *cmd, *enc1, *enc2;
    DWORD challenge, T_c_i;
    DWORD enc1_C, enc2_C, random, challenge_C;
    INT len;

    SYSTEMTIME sysTime;
    FILETIME time;
    
    if(commands_length[HELO] == Recv(sock,  msg, commands_length[HELO], FALSE)){
        
        if(0 == strcmp(commands[HELO], msg)){
            ZeroMemory(msg, commands_length[HELO]);
            
            if(RESPONSE_CODE_LENGTH == Send(sock, CONTINUE, RESPONSE_CODE_LENGTH, 0, FALSE)){
                GetSystemTime(&sysTime);
                SystemTimeToFileTime(&sysTime, &time);

                srand(time.dwLowDateTime);
                random = rand();
                challenge = token_S ^ random;
                
                CHAR randTok[MAX_TOKEN_LENGTH] = {0};
                len = sprintf(randTok, "%lu", challenge);

                if(len == Send(sock, randTok, len, 0, FALSE)){
                    if(commands_length[AUTH] + 4 <= Recv(sock, msg, commands_length[AUTH] + MAX_TOKEN_LENGTH + MAX_TOKEN_LENGTH, FALSE)){
                        cmd = strtok_s(msg, " ", &ptr);

                        if(cmd && 0 == strcmp(cmd, commands[AUTH])){
                            enc1 = strtok_s(NULL, ";", &ptr);

                            if(enc1){
                                enc1_C = strtoul(enc1, NULL, 10);
                                enc2 = strtok_s(NULL, ";", &ptr);

                                if(enc2){
                                    enc2_C = strtoul(enc2, NULL, 10);

                                    T_c_i = token_S ^ challenge ^ enc1_C;
                                    challenge_C = T_c_i ^ enc2_C;

                                    if(challenge_C == challenge) return 0;
                                }
                            }
                        }
                    }
                }
            }
        
        }
    }

    return 1;
}

DWORD WINAPI
protocol(LPVOID args){
    thread *info = (thread*) args;
    SuspendThread(info->handle);

    info = &thState[info->index];

    while(true){
        SOCKET sock = info->sock;
        
        if(0 == authentication(sock)){
            
            Send(sock, OK, RESPONSE_CODE_LENGTH, 0, FALSE);

            CHAR msg[BUFF_SIZE];
            CHAR *ptr, *cmd, *args;
            CHAR log[BUFF_SIZE] = {0};
            DWORD logLen;
            LARGE_INTEGER time;
            time.LowPart = info->timestamp.dwLowDateTime;
            time.HighPart = info->timestamp.dwHighDateTime;
            
            SetLastError(NO_ERROR);
            if(0 == recv(sock, msg, BUFF_SIZE, 0) || WSAGetLastError() != NO_ERROR)
                printf("Errore da parte del client!\n");
            else{
                printf("> %s\n", msg);
            
                cmd = strtok_s(msg, " ", &ptr);
                logLen = sprintf(log, "%lu %s %s %s %" PRId64 "\n", GetCurrentThreadId(), info->address, info->port, cmd, time.QuadPart);

                _WriteFile("file di Log", info->log, log, logLen, NULL, NULL);
            

                if(0 == strcmp(cmd, commands[LSF])){
                    args = strtok_s(NULL, " ", &ptr);

                    WIN32_FIND_DATA fileData;   

                    if(MAX_PATH < strlen(args)){
                        printf("Path non ammesso!\n");
                        Send(sock, ABORT, RESPONSE_CODE_LENGTH, 0, FALSE);
                    }

                    HANDLE find = FindFirstFile(args, &fileData);

                    if(INVALID_HANDLE_VALUE == find){
                        printf("Directory non presente!\n");
                        Send(sock, ABORT, RESPONSE_CODE_LENGTH, 0, FALSE);

                    }else{
                        if(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                            _FindClose(find);

                            args = strcat(args, "//*");
                            find = _FindFirstFile(args, &fileData);

                            CHAR out[BUFF_SIZE] = {0};
                            CHAR record[BUFF_SIZE] = {0};
                            SHORT counter = 0;
                            LARGE_INTEGER size;

                            Send(sock, CONTINUE, RESPONSE_CODE_LENGTH, 0, FALSE);

                            do{ counter++; }while(0 != _FindNextFile(find, &fileData));

                            find = _FindFirstFile(args, &fileData);

                            do{
                                counter--;
                                size.HighPart = fileData.nFileSizeHigh;
                                size.LowPart = fileData.nFileSizeLow;

                                if(0 != counter) sprintf(record, "%" PRId64 " %s \r\n", size.QuadPart, fileData.cFileName);
                                else  sprintf(record,"%" PRId64 " %s \r\n.\r\n", size.QuadPart, fileData.cFileName);

                                strcat(out, record);

                            }while(0 != _FindNextFile(find, &fileData));

                            Send(sock, out, BUFF_SIZE, 0, FALSE);
                            _FindClose(find);
                            
                        }else{
                            printf("Il path inserito non è una directory!\n");
                            _FindClose(find);
                            Send(sock, ABORT, RESPONSE_CODE_LENGTH, 0, FALSE);

                        }
                    }

                }else if(0 == strcmp(cmd, commands[EXEC])){
                    
                    DWORD cc = Strtol(strtok_s(NULL, " ", &ptr));

                    Send(sock, CONTINUE, RESPONSE_CODE_LENGTH, 0, FALSE);

                    if(0 == strcmp("printworkdir", ptr)){
                        ZeroMemory(ptr, sizeof(ptr));
                        ptr = "cd";
                    }

                    BOOL change = FALSE;
                    cmd = strtok_s(NULL, " ", &ptr);

                    if(0 == strcmp("remove", cmd)){
                        CHAR tmp[BUFF_SIZE] = {0};

                        sprintf(tmp, "del %s", ptr);
                        ZeroMemory(ptr, sizeof(ptr));
                        strcpy(ptr, tmp);
                        change = TRUE;
                    }

                    if(cc > 2 && 0 == strcmp("copy", cmd)){
                        CHAR src[cc-1][MAX_PATH];
                        CHAR *tmp, dst[MAX_PATH] = {0};
                        DWORD counter = 0;

                        while(NULL != (tmp = strtok_s(NULL, " ", &ptr))){
                            if(counter < cc-1) strcpy(src[counter], tmp);
                            else strcpy(dst, tmp);
                            counter++;
                        }

                        WIN32_FIND_DATA fileData;

                        SetLastError(NO_ERROR);
                        HANDLE find = FindFirstFile(dst, &fileData);

                        if(INVALID_HANDLE_VALUE == find){
                            CHAR error[200] = {0};

                            if(GetLastError() == ERROR_FILE_NOT_FOUND)
                                printf("%s non e' presente!\n", dst);
                                
                            else 
                                printf("Errore con il file: %s, %s\n", dst, GetErrorStri(GetLastError(), error));

                            Send(sock, "[1] \r\n.\r\n", 9, 0, FALSE);

                        }else{
                            if(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                                for(DWORD i=0; i<cc-1; i++){
                                    ZeroMemory(ptr, sizeof(ptr));

                                    counter = sprintf(ptr, "copy %s %s", src[i], dst);

                                    FILE *pipe = _popen(ptr, "r");
                                    CHAR line[BUFF_SIZE] = {0};

                                    fgets(line, BUFF_SIZE - 6, pipe);

                                    DWORD lenLine = strlen(line);
                                    INT exitCode =  _pclose(pipe);
                                    
                                    if(0 == exitCode){ 
                                        if(i < cc-2) Send(sock, strcat(line, " \r\n"), lenLine + 3, 0, FALSE);
                                        else Send(sock, strcat(line, " \r\n.\r\n"), lenLine + 6, 0, FALSE);

                                    }else{
                                        ZeroMemory(line, sizeof(line));
                                        
                                        if(i < cc-2) sprintf(line, "[%d] \r\n", exitCode);
                                        else sprintf(line, "[%d] \r\n.\r\n", exitCode);

                                        Send(sock, line, BUFF_SIZE, 0, FALSE);
                                    }
                                }
                            }else{
                                printf("Il path inserito non è una directory!\n");
                                _FindClose(find);
                                Send(sock, ABORT, RESPONSE_CODE_LENGTH, 0, FALSE);

                            }
                        }

                    }else{
                        FILE *pipe; 

                        if(!change){
                            CHAR cmd_[255] = {0};
                            sprintf(cmd_, "%s %s", cmd, ptr);
                            pipe = _popen(cmd_, "r");
                        }else 
                            pipe = _popen(ptr, "r");

                        CHAR line[BUFF_SIZE] = {0};
                        DWORD lenLine = 0;

                        while(!feof(pipe)){
                            ZeroMemory(line, lenLine);

                            fgets(line, BUFF_SIZE-1, pipe);
                            lenLine = strlen(line);
                            Send(sock, line, lenLine, 0, FALSE);
                        }

                        INT exitCode =  _pclose(pipe);

                        if(0 == exitCode)
                            Send(sock, " \r\n.\r\n", 6, 0, FALSE);
                        else{
                            ZeroMemory(line, lenLine);
                            
                            lenLine = sprintf(line, "Il comando ha prodotto un errore: [%d] \r\n.\r\n", exitCode);
                            Send(sock, line, lenLine, 0, FALSE);
                        }

                    }

                }else if(0 == strcmp(cmd, commands[DOWNLOAD])){
                    args = strtok_s(NULL, " ", &ptr);
                    args = strtok_s(args, ";", &ptr);

                    CHAR *tmp = strtok_s(NULL, ";", &ptr);
                    CHAR *fileName = args;
                    HANDLE file = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    BOOL err = FALSE;

                    if(INVALID_HANDLE_VALUE == file){
                        if(2 == GetLastError()) file = _CreateFile(fileName, GENERIC_WRITE, NULL, CREATE_NEW);
                        else{
                            CHAR error[200] = {0};

                            printf("Errore con il file: %s, %s\n", fileName, GetErrorStri(GetLastError(), error));
                            err = TRUE;
                        }
                    }
                    if(!err){

                        DWORD size = Strtol(tmp);
                        DWORD written;
                        CHAR line[255];
                        INT bufLen = 255;

                        printf("Ricevo un file dal client...\n");

                        while(0 < size){
                            ZeroMemory(line, bufLen);
                            bufLen = Recv(sock, line, 254, TRUE);
                            size -= bufLen;
                            _WriteFile(fileName, file, line, bufLen, &written, NULL);
                        }

                        Send(sock, OK, RESPONSE_CODE_LENGTH, 0, TRUE);

                        printf("File ricevuto con successo, salvato come %s\n", fileName);
                        _CloseHandle(file);
                    }

                }else if(0 == strcmp(cmd, commands[SIZE_])){
                    args = strtok_s(NULL, " ", &ptr);
                
                    HANDLE srcFile  = CreateFile(args, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    if(INVALID_HANDLE_VALUE == srcFile){
                        if(2 == GetLastError()){
                            printf("Il file %s non e' presente!\n", args);
                            Send(sock, ABORT, RESPONSE_CODE_LENGTH, 0, FALSE);
                        } 
                        else{
                            CHAR error[200] = {0};

                            Send(sock, ABORT, RESPONSE_CODE_LENGTH, 0, FALSE);
                            printf("Errore con il file: %s, %s\n", args, GetErrorStri(GetLastError(), error));
                        }
                    }
                    else{
                        DWORD size = GetFileSize(srcFile, NULL);

                        Send(sock, CONTINUE, RESPONSE_CODE_LENGTH, 0, FALSE);

                        ZeroMemory(msg, sizeof(msg));

                        DWORD lenMsg = sprintf(msg, "%lu", size);
                        
                        Send(sock, msg, lenMsg, 0, FALSE);

                        ZeroMemory(msg, sizeof(msg));

                        Recv(sock, msg, BUFF_SIZE, FALSE);

                        printf("> %s\n", msg);
                        cmd = strtok_s(msg, " ", &ptr);
                        args = strtok_s(NULL, ",", &ptr);

                        DWORD bytesReaded;

                        size = Strtol(ptr);

                        if(size < BUFF_SIZE){
                            CHAR line[size];

                            _ReadFile(args, srcFile, line, size, &bytesReaded, NULL);
                            Send(sock, line, bytesReaded, 0, FALSE);

                        }else{
                            DWORD parts = (DWORD) (size/BUFF_SIZE);
                            DWORD rem = size%BUFF_SIZE;
                            CHAR line[BUFF_SIZE];
                            
                            for(DWORD i=0; i<parts; i++){
                                _ReadFile(args, srcFile, line, BUFF_SIZE, &bytesReaded, NULL);
                                Send(sock, line, BUFF_SIZE, 0, FALSE);
                            }

                            CHAR lastLine[rem];

                            _ReadFile(args, srcFile, lastLine, rem, &bytesReaded, NULL);
                            printf("Ho inviato il file %s con successo!\n", args);
                            Send(sock, lastLine, rem, 0, FALSE);

                        }

                        CHAR response[RESPONSE_CODE_LENGTH];

                        while(0 == strcmp(OK, response)) Recv(sock, response, RESPONSE_CODE_LENGTH, FALSE);

                        _CloseHandle(srcFile);
                    }

                }else printf("Comando ricevuto non valido!\n");
            }
        
            ZeroMemory(msg, sizeof(msg));
        }else
            Send(sock, ABORT, RESPONSE_CODE_LENGTH, 0, FALSE);

        thState[info->index].state = TRUE;
        Closesocket(thState[info->index].sock);
        ReleaseSemaphore(sem, 1, NULL);
        thState[info->index].sock = INVALID_SOCKET;
        SuspendThread(info->handle);
    }

    return 0;
}

INT
main(INT argc, LPTSTR argv[]){

    WSADATA data;
    _WSAStartup(&data);

    SHORT p, n, c, s, l;
    p = n = c = s = l = 0;

    LPSTR confPath = "";
    LPSTR logPath = "tmp/server.log";

    DWORD threads = 10;
    SHORT portTCP = 8888;

    for(INT i=1; i<argc; i++){
            if('-' == argv[i][0]){
                if(2 == lstrlen(argv[i])){
                    switch(argv[i][1]){
                        case 'p':
                            if(p == 0) p++;
                            else exitMsg("Errore, argomenti non validi!\n");
                        break;

                        case 'n':
                            if(n == 0) n++;
                            else exitMsg("Errore, argomenti non validi!\n");
                        break;

                        case 'c':
                            if(c == 0) c++;
                            else exitMsg("Errore, argomenti non validi!\n");
                        break;

                        case 's':
                            if(s == 0) s++;
                            else exitMsg("Errore, argomenti non validi!\n");
                        break;

                        case 'l':
                            if(l == 0) l++;
                            else exitMsg("Errore, argomenti non validi!\n");
                        break;

                        default: 
                            exitMsg("Errore, comando inesistente!\n");
                    }
                }else exitMsg("Errore! \n");
            }else{
                    if(p == 1){
                        portTCP = Strtol(argv[i]);
                        p++;

                    }else if(n == 1){
                        threads = Strtol(argv[i]);
                        n++;

                    }else if(c == 1){
                        confPath = argv[i];
                        c++;

                    }else if(l == 1){
                        INT sLen = lstrlen(argv[i]);

                        if(sLen >= 5){
                            char *subS = strstr(argv[i], ".log");
                            if(NULL == subS || subS != &argv[i][sLen -4]) exitMsg("Estensione file di log non valida!\n");

                        }else exitMsg("Estensione file di log non valida!\n");

                        logPath = argv[i];
                        l++;
                    }else exitMsg("Errore! \n");
                }
    }

    if(p == 1 || c == 1 || l == 1 || n == 1) exitMsg("Errore, argomenti non validi!\n");

    if(0 != strcmp(confPath, "")){
        HANDLE confFile = _CreateFile(confPath, GENERIC_READ, NULL, OPEN_EXISTING);
        CHAR line[255];
        DWORD bytesReaded;

        _ReadFile(confPath, confFile, line, 254, &bytesReaded, NULL);

        if(bytesReaded <= 254) line[bytesReaded] = '\0';

        CHAR *part = strtok(line, "=");

        while(NULL != part){
            if(0 == lstrcmp("STDOUT_TOKEN", part)){
                if(0 == s){
                    part = strtok(NULL, "\n");
                    SHORT sTemp = Strtol(part);

                    if(0 == sTemp || 1 == sTemp) s = sTemp;
                    else exitMsg("Errore in file di configurazione!\n");
                }

            }else if(0 == lstrcmp("PORT_NUMBER", part)){
                if(0 == p){
                    part = strtok(NULL, "\n");

                    portTCP = Strtol(part);
                }

            }else if(0 == lstrcmp("THREADS_NUMBER", part)){
                if(0 == n){
                    part = strtok(NULL, "\n");

                    threads = Strtol(part);
                }

            }else exitMsg("Errore in file di configurazione!\n");

            part = strtok(NULL, "=");
        }

        _CloseHandle(confFile);
    }

    HANDLE logFile = CreateFile(logPath, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(ERROR_FILE_NOT_FOUND == GetLastError()){
        logFile = CreateFile(logPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    _CloseHandle(logFile);

    printf("N threads: %lu\nPorta TCP: %d\nConf path: %s\nLog path: %s\noutToken: %d\n", threads, portTCP, confPath, logPath, s);

    CHAR passPhrase[MAX_TOKEN_LENGTH];

    printf("Inserire passPhrase: ");
    scanf("%255s", passPhrase); // prende i primi 255 = MAX_TOKEN_LENGTH

    fflush(stdin);

    token_S = generateToken(passPhrase);

    if(1 == s) printf("Server token: %lu\n", token_S);
    printf("\n");

    SOCKET listenSock, clientSock = INVALID_SOCKET;
    struct sockaddr_in addr, clientAddr;

    listenSock = Socket(AF_INET, SOCK_STREAM, 0);

    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portTCP);
    addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    Bind(listenSock, (struct sockaddr *) &addr, sizeof(addr));

    BOOL running = TRUE;
    DWORD count = 0;

    Listen(listenSock, threads);

    thState = malloc(sizeof(thread) * threads);

    for(DWORD i=0; i<threads; i++)
        thState[i].state = TRUE;

    SYSTEMTIME sysTime;
    FILETIME time;
    socklen_t clientAddrLen = sizeof(clientAddr);
    DWORD readyThreads = threads;

    logFile = _CreateFile(logPath, FILE_APPEND_DATA, NULL, OPEN_EXISTING);
    sem = _CreateSemaphore(NULL, threads, threads, NULL);

    for(DWORD i=0; i<threads; i++){
        thState[i].index = i;
        thState[i].sock = INVALID_SOCKET;
        thState[i].handle = _CreateThread(0, protocol, &thState[i], 0, 0);
    }

    while(running){
        if(count < threads*6){
             
            clientSock = accept(listenSock, (struct sockaddr *) &clientAddr, &clientAddrLen);
            _WaitForSingleObject(sem, INFINITE);
            readyThreads--;

            if(INVALID_SOCKET != clientSock){
                
                for(DWORD i=0; i<threads; i++){

                    if(thState[i].state && thState[i].sock == INVALID_SOCKET){
                        GetSystemTime(&sysTime);
                        SystemTimeToFileTime(&sysTime, &time);
                        
                        thState[i].state = FALSE;
                        strcpy(thState[i].address, inet_ntoa(clientAddr.sin_addr));
                        thState[i].sock = clientSock;
                        thState[i].log = logFile;
                        thState[i].timestamp = time;
                        sprintf(thState[i].port, "%d", clientAddr.sin_port);
                        ResumeThread(thState[i].handle);
                        count++;
                        break;

                    }
                }
            }else
                exitMsgWSA("Errore durante accept", WSAGetLastError());
        }else{
            BOOL active = FALSE;

            for(DWORD i=0; i<threads; i++)
                if(!thState[i].state) active = TRUE;

            if(!active)
                running = FALSE;
        }
    }

    _CloseHandle(sem);
    _CloseHandle(logFile);
    Closesocket(listenSock); 

    _WSACleanup();
    exit(0);
}
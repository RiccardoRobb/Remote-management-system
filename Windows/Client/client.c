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

#ifndef _FUN_H
#include "./../functions.h"
#endif

static DWORD token_S, token_C;

// client comm: -l <path> = ls su path, -e <cmd [args...]> = esegui comando, -d <src-path> <dst-path> = download, -u <src-path> <dst-path> = upload
INT authentication(SOCKET sock){
    CHAR msg[BUFF_SIZE] = {0};
    DWORD challange;
    DWORD enc1, enc2;
    INT len;

    if(commands_length[HELO] == Send(sock,  commands[HELO], commands_length[HELO], 0, TRUE)){
        if(RESPONSE_CODE_LENGTH == Recv(sock, msg, RESPONSE_CODE_LENGTH, TRUE)){
            if(0 == strcmp(CONTINUE, msg)){

                ZeroMemory(msg, sizeof(msg));

                if(0 != Recv(sock, msg, MAX_TOKEN_LENGTH, TRUE)){
                    challange = Strtol(msg);
                    challange = token_S ^ challange;
                    enc1 = token_S ^ challange ^ token_C;
                    enc2 = token_C ^ challange;

                    ZeroMemory(msg, sizeof(msg));

                    len = sprintf(msg, "%s %lu;%lu", commands[AUTH], enc1, enc2);

                    if(len == Send(sock,  msg, len, 0, TRUE)){
                        ZeroMemory(msg, sizeof(msg));

                        if(RESPONSE_CODE_LENGTH == Recv(sock, msg, RESPONSE_CODE_LENGTH, TRUE)){
                            if(0 == strcmp(msg, OK)) return 0;
                        }
                    }             
                }
            }
        }
    }


    return 1;
}

VOID
protocol(SOCKET sock){
    if(0 != authentication(sock)) exitMsg("Autenticazione non riuscita!\n");
    printf("Autenticazione riuscita!\n");
}

INT
main(INT argc, LPTSTR argv[]){

    WSADATA data;
    _WSAStartup(&data);

    CHAR *serverIP;
    INT portTCPServer;
    SHORT h, p, l, e, d, u;
    h = p = l = e = d = u = 0;

    CHAR *command, *args;

    if(argc < 7) exitMsg("Argomenti non validi!\n");

    for(INT i=1; i<argc; i++){
        if('-' == argv[i][0]){
            if(2 == strlen(argv[i])){
                switch(argv[i][1]){
                    case 'p':
                        if(0 == p) p++;
                        else exitMsg("Argomenti inseriti non validi!\n");
                    break;

                    case 'h':
                        if(0 == h) h++;
                        else exitMsg("Argomenti inseriti non validi!\n");
                    break;

                    case 'l':
                        if(0 == l) l++;
                        else exitMsg("Comandi inseriti non validi!\n");
                    break;

                    case 'e':
                        if(0 == e) e++;
                        else exitMsg("Comandi inseriti non validi!\n");
                    break;
                    
                    case 'd':
                        if(0 == d) d++;
                        else exitMsg("Comandi inseriti non validi!\n");
                    break;
                    
                    case 'u':
                        if(0 == u) u++;
                        else exitMsg("Comandi inseriti non validi!\n");
                    break;
                }
            }else exitMsg("Argomenti inseriti non validi!\n");
        }else{
            if(h == 1){
                serverIP = argv[i];
                SHORT lenIP = (SHORT) lstrlen(serverIP);
                SHORT ckIP = 0;
                SHORT pCount = 0;

                if(15 < lenIP || 7 > lenIP) exitMsg("Indirizzo IP server non valido!\n");

                for(SHORT i=0; i<lenIP; i++){
                    if(46 == serverIP[i] && 0 != ckIP){
                        ckIP = 0;
                        pCount++;
                    }
                    else if(57 < serverIP[i] || 48 > serverIP[i] || 2 < ckIP) exitMsg("Errore in indirizzo IP!\n");
                    else ckIP++;
                }

                if(3 != pCount) exitMsg("Errore in indirizzo IP!\n");

                h++;

            }else if(p == 1){
                if(lstrlen(argv[i]) > 4) exitMsg("Errore in porta TCP server!\n");

                portTCPServer = Strtol(argv[i]);
                p++;

            }else if(l == 1){
                command = "-l";
                args = argv[i];
                l++;

                if(i != argc-1) exitMsg("Argomenti inseriti non validi!\n");

            }else if(e == 1){
                command = "-e";
                args = argv[i];

                if(i < argc){
                    for(int j=i+1; j<argc; j++){
                        args = strcat(args, " ");
                        args = strcat(args, argv[j]);
                    }
                    e++;
                    break;

                }else exitMsg("Argomenti inseriti non validi!\n");

            }else if(d == 1){
                command = "-d";
                args = argv[i];

                if(i < argc-1){
                    for(int j=i+1; j<argc; j++){
                        args = strcat(args, " ");
                        args = strcat(args, argv[j]);
                        d++;
                    }
                    if(d != 2) exitMsg("Argomenti inseriti non validi!\n");
                    break;

                }else exitMsg("Argomenti inseriti non validi!\n");

            }else if(u == 1){
                command = "-u";
                args = argv[i];

                if(i < argc-1){
                    for(int j=i+1; j<argc; j++){
                        args = strcat(args, " ");
                        args = strcat(args, argv[j]);
                        u++;
                    }
                    if(u != 2) exitMsg("Argomenti inseriti non validi!\n");
                    break;

                }else exitMsg("Argomenti inseriti non validi!\n");
                
            }else exitMsg("Argomenti inseriti non validi!\n");
        }
    }

    printf("ServerIP: %s, serverPort: %d\n", serverIP, portTCPServer);

    CHAR serverPassPhrase[MAX_TOKEN_LENGTH], clientPassPhrase[MAX_TOKEN_LENGTH];

    printf("Inserire server passPhrase:\n");
    scanf("%255s", serverPassPhrase);

    fflush(stdin);

    printf("Inserire client passPhrase:\n");
    scanf("%255s", clientPassPhrase);
    printf("\n");

    fflush(stdin);

    token_S = generateToken(serverPassPhrase);
    token_C = generateToken(clientPassPhrase);
    
    SOCKET connectSock;

    connectSock = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serverInfo;
    serverInfo.sin_addr.S_un.S_addr = inet_addr(serverIP);
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(portTCPServer);

    Connect(connectSock, (struct sockaddr *) &serverInfo, sizeof(serverInfo));

    protocol(connectSock);

    CHAR msg[BUFF_SIZE] = {0};
    SHORT lenMsg;

    switch(command[1]){
        case 'l':
            lenMsg = sprintf(msg, "%s %s", commands[LSF], args);

            Send(connectSock, msg, lenMsg, 0, TRUE);

            ZeroMemory(msg, lenMsg);

            if(RESPONSE_CODE_LENGTH == Recv(connectSock, msg, RESPONSE_CODE_LENGTH, TRUE)){
                if(0 == strcmp(msg, ABORT)) exitMsg("Il comando LSF ha prodotto un errore!\n");
                else if(0 == strcmp(msg, CONTINUE)){
                    
                    ZeroMemory(msg, sizeof(msg));

                    while(NULL == strstr(msg, "\r\n.\r\n")){
                        Recv(connectSock, msg, BUFF_SIZE, TRUE);

                        printf("%s", msg);
                    }
                }
            }
        break;

        case 'e': ;
            SHORT cc = 0;

            for(USHORT i=0; i<strlen(args); i++) 
                if(' ' == args[i]) cc++;
            
            lenMsg = sprintf(msg, "%s %d %s", commands[EXEC], cc, args);

            Send(connectSock, msg, lenMsg, 0, TRUE);

            ZeroMemory(msg, lenMsg);

            lenMsg = Recv(connectSock, msg, RESPONSE_CODE_LENGTH, TRUE);

            if(0 == strcmp(ABORT, msg)){
                printf("Errore con gli argomenti passati alla copy!\n");
                break;
            }

            if(0 == strcmp(msg, CONTINUE)){

                printf("Ricevo: \n");

                while(NULL == strstr(msg, " \r\n.\r\n")){
                    ZeroMemory(msg, lenMsg);
                    
                    lenMsg = recv(connectSock, msg, BUFF_SIZE, 0);
                    printf("%s", msg);
                }

            }else exitMsg("Errore di comunicazione con il server!\n");

        break;

        case 'd':
            Strtok(args, " ");

            CHAR *fileName = args;
            HANDLE srcFile  = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            
            if(INVALID_HANDLE_VALUE == srcFile){
                CHAR error[200] = {0};

                printf("Errore con il file: %s, %s\n", fileName, GetErrorStri(GetLastError(), error));
                exit(1);
            }
            
            DWORD size = GetFileSize(srcFile, NULL);

            args = Strtok(NULL, " ");
            lenMsg = sprintf(msg, "%s %s;%lu", commands[DOWNLOAD], args, size);

            Send(connectSock, msg, lenMsg, 0, TRUE);

            DWORD bytesReaded;

            printf("Invio il file %s al server...\n", fileName);

            if(size < BUFF_SIZE){
                CHAR line[size];

                _ReadFile(fileName, srcFile, line, size, &bytesReaded, NULL);
                Send(connectSock, line, bytesReaded, 0, TRUE);

            }else{
                DWORD parts = (DWORD) (size/BUFF_SIZE);
                DWORD rem = size%BUFF_SIZE;
                CHAR line[BUFF_SIZE];
                
                for(DWORD i=0; i<parts; i++){
                    _ReadFile(fileName, srcFile, line, BUFF_SIZE, &bytesReaded, NULL);
                    Send(connectSock, line, BUFF_SIZE, 0, TRUE);
                }

                CHAR lastLine[rem];

                _ReadFile(fileName, srcFile, lastLine, rem, &bytesReaded, NULL);
                Send(connectSock, lastLine, rem, 0, TRUE);
            }

            CHAR response[RESPONSE_CODE_LENGTH];

            while(0 == strcmp(OK, response)) Recv(connectSock, response, RESPONSE_CODE_LENGTH, TRUE);

            printf("Ho inviato con successo il file!\n");
            _CloseHandle(srcFile);
            
        break;

        case 'u': ;
            CHAR *src = Strtok(args, " ");
            CHAR *dst = Strtok(NULL, " ");

            lenMsg = sprintf(msg, "%s %s", commands[SIZE_], src);

            Send(connectSock, msg, lenMsg, 0, TRUE);

            ZeroMemory(msg, lenMsg);

            lenMsg = Recv(connectSock, msg, RESPONSE_CODE_LENGTH, TRUE);

            if(0 == strcmp(msg, ABORT)) exitMsg("Il comando UPLOAD ha prodotto un errore!\n");
            else{
                HANDLE file = CreateFile(dst, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                if(INVALID_HANDLE_VALUE == file){
                    if(2 == GetLastError()) file = CreateFile(dst, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                    
                    if(INVALID_HANDLE_VALUE == file){
                        CHAR error[200] = {0};

                        printf("Errore con il file: %s, %s\n", dst, GetErrorStri(GetLastError(), error));
                        exit(1);
                    }
                }

                CHAR sizeFile[BUFF_SIZE] = {0};

                ZeroMemory(msg, lenMsg);

                Recv(connectSock, sizeFile, BUFF_SIZE, TRUE);

                lenMsg = sprintf(msg, "UPLOAD %s,%s", src, sizeFile);

                printf("Ricevo il file %s dal server...\n", src);

                Send(connectSock, msg, lenMsg, 0, TRUE);

                DWORD written;
                DWORD size = Strtol(sizeFile);
                CHAR line[255];
                lenMsg = 255;

                while(0 < size){
                    ZeroMemory(line, lenMsg);
                    lenMsg = Recv(connectSock, line, 254, TRUE);
                    size -= lenMsg;
                    _WriteFile(dst, file, line, lenMsg, &written, NULL);
                }

                Send(connectSock, OK, RESPONSE_CODE_LENGTH, 0, TRUE);

                printf("Ho ricevuto con successo il file, salvato come: %s\n", dst);
                _CloseHandle(file);
            }

        break;
    }
    
    Closesocket(connectSock);
 
    _WSACleanup();
    exit(0);    
}
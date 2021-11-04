#ifndef _WINDOWS_H
#include <windows.h>
#endif

#ifndef _STDIO_H
#include <stdio.h>
#endif

#include "functions.h"

// Suppress warnings

VOID
removeUnusedWarning(){
    (void) commands;
    (void) commands_length;
}

// Functions

DWORD
generateToken(LPTSTR passphrase)
{
    DWORD token = 1;
    SHORT passphraseLength = lstrlen(passphrase);

    for (SHORT i = 0; i < passphraseLength; i++)
        token += passphrase[i] * (token % MAX_MUL_TOKEN);

    token += passphraseLength;
    
    return token;
}

CHAR *
strtok_s(CHAR *s, const CHAR *delim, CHAR **save_ptr)
{
  CHAR *end;
  if (s == NULL)
    s = *save_ptr;
  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }

  s += strspn (s, delim);
  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }

  end = s + strcspn (s, delim);
  if (*end == '\0')
    {
      *save_ptr = end;
      return s;
    }

  *end = '\0';
  *save_ptr = end + 1;
  return s;
}

VOID
exitMsg(LPTSTR msg){
    printf(msg);
    WSACleanup();
    exit(1);
}

BOOL
GetErrorMessage(DWORD errorCode, LPTSTR buffer, DWORD buffLen){
    if(buffLen == 0){
            return FALSE;
    }

    DWORD msg = FormatMessage(  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,  
                                errorCode,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                buffer,
                                buffLen,
                                NULL );

    return (msg = 0);
}

LPTSTR
GetErrorStri(DWORD errorCode, LPTSTR error){
    printf("[%lu]\n", errorCode);
    GetErrorMessage(errorCode, error, 200);

    return error; 
}

VOID
exitMsgWSA(CHAR *msg, int errNo){
    CHAR error[200] = {0};

    printf("%s, %s\n", msg, GetErrorStri(errNo, error));
    WSACleanup();
    exit(1);
}

VOID
exitMsgErr(CHAR *msg){
    CHAR error[200] = {0};

    printf("%s, %s\n", msg, GetErrorStri(GetLastError(), error));
    WSACleanup();
    exit(1);
}

// Wrappers 

LPTSTR
Strtok(LPTSTR source, LPTSTR del){
    LPTSTR out = strtok(source, del);

    if(NULL == out)
        exitMsg("Errore in fase di split stringa!\n");

    return out;
}

DWORD
Strtol(LPTSTR buff){
    LPTSTR endptr;

    DWORD out = (DWORD) strtoll(buff, &endptr, 10);

    if(endptr == buff || '\0' != *endptr)
        exitMsg("Valore intero non valido!\n");
    
    return out;
}

HANDLE
_CreateFile(CHAR *fileName, DWORD mode, LPSECURITY_ATTRIBUTES secAttr, DWORD creationMode){
    SetLastError(NO_ERROR);

    HANDLE out = CreateFile(fileName, mode, 0, secAttr, creationMode, FILE_ATTRIBUTE_NORMAL, NULL);

    if(INVALID_HANDLE_VALUE == out || GetLastError() != NO_ERROR){
        CHAR error[200] = {0};

        sprintf(error, "Errore con il file file: %s", fileName);
        exitMsgErr(error);
    }

    return out;
}

VOID
_ReadFile(LPTSTR fileName, HANDLE file, LPVOID buffer, DWORD nByte, LPDWORD nByteRead, LPOVERLAPPED overlapped){
    if(!ReadFile(file, buffer, nByte, nByteRead, overlapped)){
        CHAR error[200] = {0};

        sprintf(error, "Errore in fase di lettura file: %s", fileName);
        exitMsgErr(error);
    }
}

VOID
_WriteFile(LPTSTR fileName, HANDLE file, LPVOID buffer, DWORD nByte, LPDWORD nByteWritten, LPOVERLAPPED overlapped){
    if(!WriteFile(file, buffer, nByte, nByteWritten, overlapped)){
        CHAR error[200] = {0};

        sprintf(error, "Errore in fase di scrittura file: %s", fileName);
        exitMsgErr(error);
    }
}

VOID 
_CloseHandle(HANDLE file){
    if(0 == CloseHandle(file))
        exitMsgErr("Errore durante chiusura file");
}

HANDLE
_CreateThread(DWORD stackSize, LPTHREAD_START_ROUTINE routine, PVOID param, DWORD creationFlag, PDWORD tid){
    HANDLE out = CreateThread(NULL, stackSize, routine, param, creationFlag, tid);

    if(NULL == out)
       exitMsgErr("Errore durante creazione thread");

    return out;
}

HANDLE
_CreateSemaphore(LPSECURITY_ATTRIBUTES attr, LONG startValue, LONG maxValue, LPCSTR name){
    SetLastError(NO_ERROR);

    HANDLE out = CreateSemaphore(attr, startValue, maxValue, name);

    if(NULL == out || NO_ERROR != GetLastError())
        exitMsgErr("Errore durante creazione semaforo");

    return out;
}

BOOL
_WaitForSingleObject(HANDLE object, DWORD millis){
    DWORD out = WaitForSingleObject(object, millis);

    if(WAIT_ABANDONED == out) 
        return FALSE;
    else if(WAIT_FAILED == out)
        exitMsgErr("Errore durante Wait");

    return TRUE;
}

BOOL
_GetExitCodeThread(HANDLE th, PDWORD code){
    if(0 == GetExitCodeThread(th, code))
        exitMsgErr("Errore durante reperimento codice di uscita thread"); 

    if(STILL_ACTIVE == *code) return FALSE;

    return TRUE;
}

VOID
_WSAStartup(LPWSADATA data){
    DWORD ver = MAKEWORD(2, 2); // Versione utilizzata = 2.2
    DWORD errNo = WSAStartup(ver, data);

    if(0 != errNo)
        exitMsgErr("Errore durante caricamento DLL");
}

VOID
_WSACleanup(){  // Errore WSA 10091, limitare che più di un WINSOCK.dll sia caricato (attivato con Ctrl+c)
    if(0 != WSACleanup())
        exitMsgWSA("Errore durante chiusura DLL", WSAGetLastError());
}

VOID
Closesocket(SOCKET sock){
    if(0 != closesocket(sock))
        exitMsgWSA("Errore durante chiusura socket", WSAGetLastError());
}

SOCKET
Socket(INT domain, INT type, INT protocol){
    SOCKET sock = socket(domain, type, protocol);

    if(INVALID_SOCKET == sock)
        exitMsgWSA("Errore durante creazione socket", WSAGetLastError());

    return sock;
}

INT
Send(SOCKET sock, const CHAR *buf, INT len, INT flag, BOOL localError){
    INT out = send(sock, buf, len, flag);

    if(SOCKET_ERROR == out){
        if(localError)
            exitMsgWSA("Errore durante invio dati", WSAGetLastError());
        else{
            printf("Errore durante invio dati!\n");
            ExitThread(1);
        }
    }

    return out;
}

VOID
Bind(SOCKET sock, struct sockaddr *addr, INT addrLen){
    if(SOCKET_ERROR == bind(sock, addr, addrLen)){
        free(addr);
        Closesocket(sock);
        exitMsgWSA("Errore durante 'binding' socket", WSAGetLastError());
    }
}

VOID
Listen(SOCKET sock, INT nConns){
    if(SOCKET_ERROR == listen(sock, nConns)){
        INT errNo = WSAGetLastError();

        Closesocket(sock);
        exitMsgWSA("Errore durante 'listening' socket", errNo);
    }
}

INT
Recv(SOCKET sock, CHAR *buf, INT bufLen, BOOL localError){
    SetLastError(NO_ERROR);
    INT numC = recv(sock, buf, bufLen, 0);
    INT errNo = WSAGetLastError();

    if(errNo != NO_ERROR){
        if(localError){
            Closesocket(sock);
            exitMsgWSA("Errore durante 'recv' socket", errNo);
        }else{
            printf("Errore durante 'recv' socket!\n");
            ExitThread(1);
        }
    }

    if(0 == numC){
        if(localError)
            exitMsg("Errore, la socket dove si prova a leggere e' chiusa!\nOppure non ci sono messaggi da leggere!\n");
        else{
            printf("Errore, la socket dove si prova a leggere e' chiusa!\nOppure non ci sono messaggi da leggere!\n");
            ExitThread(1);
        }
    } 

    return numC;
}

VOID
Connect(SOCKET sock, struct sockaddr *addr, INT addrLen){
    if(0 != connect(sock, addr, addrLen))
        exitMsgWSA("Errore durante 'connect' socket!", WSAGetLastError());
}

INT
_FindNextFile(HANDLE file, LPWIN32_FIND_DATA fileData){
    if(0 == FindNextFile(file, fileData)){
        if(18 == GetLastError()) return 0;

        _CloseHandle(file);
        exitMsgErr("Errore ricerca file (next)");
    }

    return 1;
}

HANDLE
_FindFirstFile(CHAR *path, LPWIN32_FIND_DATA fileData){
    HANDLE out = FindFirstFile(path, fileData);

    if(INVALID_HANDLE_VALUE == out)
        exitMsgErr("Errore ricerca file (first)");

    return out;
}

VOID
_FindClose(HANDLE file){
    if(0 == FindClose(file))
        exitMsgErr("Errore in fase di chiudura dell'handle di ricerca");
}
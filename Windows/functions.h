#ifndef _FUN_H
#define _FUN_H

#define MAX_TOKEN_LENGTH 255
#define MAX_MUL_TOKEN 1240000
#define MAX_PORT_NUMBER 65535
#define BUFF_SIZE 65535

#define DELIMITER_SPACE " "

#define OK "200"
#define CONTINUE "300"
#define ABORT "400"
#define RESPONSE_CODE_LENGTH 3

#define NUMBER_OF_COMMANDS 7

typedef enum {
    HELO,
    AUTH,
    LSF,
    EXEC,
    DOWNLOAD,
    SIZE_,    // SIZE Già definito in winAPI! 
    UPLOAD
} COMMAND;

static const CHAR *commands[NUMBER_OF_COMMANDS] = { "HELO", "AUTH", "LSF", "EXEC", "DOWNLOAD", "SIZE", "UPLOAD" };
static const INT commands_length[NUMBER_OF_COMMANDS] = { 4, 4, 3, 4, 8, 4, 6 };

VOID removeUnusedWarning();

DWORD generateToken(LPTSTR passphrase);
CHAR * strtok_s(CHAR *s, const CHAR *delim, CHAR **save_ptr);
VOID exitMsg(LPTSTR msg);
BOOL GetErrorMessage(DWORD errorCode, LPTSTR buffer, DWORD buffLen);
LPTSTR GetErrorStri(DWORD errorCode, LPTSTR error);
VOID exitMsgWSA(CHAR *msg, int errNo);
VOID exitMsgErr(CHAR *msg);

LPTSTR Strtok(LPTSTR source, LPTSTR del);
DWORD Strtol(LPTSTR buff);
HANDLE _CreateFile(CHAR *fileName, DWORD mode, LPSECURITY_ATTRIBUTES secAttr, DWORD creationMode);
VOID _ReadFile(LPTSTR fileName, HANDLE file, LPVOID buffer, DWORD nByte, LPDWORD nByteRead, LPOVERLAPPED overlapped);
VOID _WriteFile(LPTSTR fileName, HANDLE file, LPVOID buffer, DWORD nByte, LPDWORD nByteWritten, LPOVERLAPPED overlapped);
VOID _CloseHandle(HANDLE file);
HANDLE _CreateThread(DWORD stackSize, LPTHREAD_START_ROUTINE routine, PVOID param, DWORD creationFlag, PDWORD tid);
HANDLE _CreateSemaphore(LPSECURITY_ATTRIBUTES attr, LONG startValue, LONG maxValue, LPCSTR name);
BOOL _WaitForSingleObject(HANDLE object, DWORD millis);
BOOL _GetExitCodeThread(HANDLE th, PDWORD code);
VOID _WSAStartup(LPWSADATA data);
VOID _WSACleanup();
VOID Closesocket(SOCKET sock);
SOCKET Socket(INT domain, INT type, INT protocol);
INT Send(SOCKET sock, const CHAR *buf, INT len, INT flag, BOOL localError);
VOID Bind(SOCKET sock, struct sockaddr *addr, INT addrLen);
VOID Listen(SOCKET sock, INT nConns);
INT Recv(SOCKET sock, CHAR *buf, INT bufLen, BOOL localError);
VOID Connect(SOCKET sock, struct sockaddr *addr, INT addrLen);
INT _FindNextFile(HANDLE file, LPWIN32_FIND_DATA fileData);
HANDLE _FindFirstFile(CHAR *path, LPWIN32_FIND_DATA fileData);
VOID _FindClose(HANDLE file);

#endif
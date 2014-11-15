// webmate.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <tchar.h>
#include "md5.h"
#include "md5sum.h"
#include <conio.h>
// MAX_CLIENTS means the number of clients that can connect while the socket is being used. 
// That means that these clients will have to wait until all clients before him have been dealt with. 
// If you specify a backlog of 5 and seven people try to connect, then the last 2 will receive an error message and should try to connect again later. 
// Usually a backlog between 2 and 10 is good, depending on how many users are expected on a server.
#define MAX_CLIENTS 15           
class Client
{
public:
  void run();
  int index;
  SOCKET s;
  sockaddr address;
} g_client[MAX_CLIENTS];


SOCKET createBindAndListenSocket()
{
  // used to store information about WinSock version
  WSADATA w;
  int error = WSAStartup (0x0202, &w);   // version 2.2
  if (error)
  { // there was an error
    return 0;
  }
  if (w.wVersion != 0x0202)
  { // wrong WinSock version!
    WSACleanup (); // unload ws2_32.dll
    return 0;
  }

  // create socket s
  SOCKET s = socket(AF_INET, SOCK_STREAM, 0); // family, type (SOCK_STREAM is TCP/IP) and protocol

  // bind s to a port
  // Note that you should only bind server sockets, not client sockets
  sockaddr_in addr; // the address structure for a TCP socket

  addr.sin_family = AF_INET;      // Address family Internet
  addr.sin_port = htons (8080);   // Assign port 8080 to this socket

  addr.sin_addr.s_addr = htonl(INADDR_ANY);   // No destination.. or inet_addr ("129.42.12.241")
  if (bind(s, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
  { // error
    WSACleanup ();  // unload WinSock
    return 0;         // quit
  }

  // now listen
  if (listen(s, MAX_CLIENTS)==SOCKET_ERROR)
  { // error!  unable to listen
    WSACleanup ();  // unload WinSock
    return false;
  }
  return s;
}
inline void endian_swap(unsigned int& x)
{
  x = (x>>24) | 
    ((x<<8) & 0x00FF0000) |
    ((x>>8) & 0x0000FF00) |
    (x<<24);
}

// shared memory, hope this isn't locked
static char g_input[MAX_CLIENTS][4];  
static int g_numClients = 0;


extern MD5state *nil;
bool replaceString(char *buffer, const char *token, const char *string)
{
  int maxSize = strlen(buffer);
  char *start = strstr(buffer, token);
  if (!start)
    return false;
  char end[400];
  strncpy(end, start + strlen(token), 400);
  *start = 0; // clip the string short so concatenation works
  strncat(buffer, string, maxSize);
  strncat(buffer, end, maxSize);
  return true;
}

DWORD WINAPI runThread(LPVOID args)
{
  Client *client = (Client *)args;
  client->run();
  return 0;
}
void Client::run()
{
  // loop and do stuff permanently here, in particular receives and sends
  // SOCKET s is initialized
  char buffer[400];
  recv(s, buffer, sizeof(buffer), 0);
  if (!strncmp(buffer, "GET ", 4))
  {
/* should be : 
GET /demo HTTP/1.1
Host: example.com
Connection: Upgrade
Sec-WebSocket-Key2: 12998 5 Y3 1  .P00
Sec-WebSocket-Protocol: sample
Upgrade: WebSocket
Sec-WebSocket-Key1: 4 @1  46546xW%0l 1 5
Origin: http://example.com
*/
    char *buf = &buffer[4];
    char dir[32];
    strncpy(dir, buffer+4, sizeof(dir));
    *strstr(dir, " ") = 0; // terminate it
    char origin[80];
    strncpy(origin, strstr(buffer, "Origin: ")+8, sizeof(origin));
    *strstr(origin, "\r") = 0; // end the string
    char location[80];
    strncpy(location, strstr(buffer, "Host: ")+6, sizeof(location));
    *strstr(location, "\r") = 0; // end the string
  /*  if (strstr(origin, "http://"))
      strcpy(location, origin + 7);
    else
      strcpy(location, "localhost:8080"); // if it is null then we assume localhost
*/
    // OK this next bit is insane, first we get the string after the websocket-Key1
    char *key[2] = { strstr(buffer, "WebSocket-Key1: ")+16, strstr(buffer, "WebSocket-Key2: ")+16 };
    unsigned int number[52];
    for (int j = 0; j<2; j++)
    {
      // then we concatenate all the numbers
      int length = (int)(strstr(key[j], "\r\n") - key[j]);
      char newNum[30];
      int ind = 0;
      int numSpaces = 0;
      for (int i = 0; i<length; i++)
      {
        if (key[j][i]>='0' && key[j][i]<='9')
          newNum[ind++] = key[j][i];
        if (key[j][i]==' ')
          numSpaces++;
      }
      newNum[ind] = 0;
      unsigned long longNum = strtoul(newNum, NULL, 10);
      number[j] = (unsigned int)longNum; // check that newNum doesn't get larger than about 4,000,000,000
      number[j] /= numSpaces; // if spaces = 0 then we have a handshake problem
      endian_swap(number[j]); // make it big-endian
    }
    // then we find the last 8 characters
    char *stringEnd = strstr(buffer, "\r\n\r\n"); // presumably the last 8 comes before the ending double CRLF (carriage return, line feed)
    number[2] = *(unsigned int *)(stringEnd + 4);
    number[3] = *(unsigned int *)(stringEnd + 8);


    MD5_CTX context;
    unsigned char digest[22];
    MD5Init (&context);
    MD5Update (&context, (unsigned char *)&number[0], 16);
    MD5Final (digest, &context);

    // below should have exactly the same result in digest2 as digest
/*    // lastly get the MD5 sum of this 128bit data 
    MD5state *state = nil;
    byte digest2[18];
    state = md5((byte *)&number[0], 16, digest2, state); 
*/

    char *response[5]     = { "HTTP/1.1 101 Web Socket Protocol Handshake\r\n",
                              "Upgrade: WebSocket\r\n",
                              "Connection: Upgrade\r\n",
                              "Sec-WebSocket-Origin: <origin>\r\n", 
                              "Sec-WebSocket-Location: ws://<location><dir>\r\n\r\n"}; 

    char actualResponse[400] = "";
    for (int i = 0; i<5; i++)
      strcat(actualResponse, response[i]);
    replaceString(actualResponse, "<origin>", origin);
    replaceString(actualResponse, "<location>", location);
    replaceString(actualResponse, "<dir>", dir);
    int what = strlen(actualResponse);
    for (int i = 0; i<what; i++)
    {
      if (actualResponse[i] & 128)
      {
        int h = 3; // BAD, need to UTF8 the string
        h++;
      }
    }
    strncat(actualResponse, (char *)digest, 16);
    int len = strlen(actualResponse);
    int amount = send(s, actualResponse, len, 0);
    amount++;
    
    char getGoing[5] = {0, '0', ',', '0', 255};
    send(s, getGoing, 5, 0);

    for(;;)
    {
      int receivedSize = 0;
      receivedSize = recv(s, buffer, sizeof(buffer), 0);
 /*     if (receivedSize != 3) // bad
      {
        int h = 0;
        h++;
        return;
      }*/
      buffer[receivedSize-1] = 0; //terminate string
      strcpy(g_input[index], &buffer[1]);

      // now create reply
      buffer[0] = ' ';
      buffer[1] = 0; // terminate
      strcat(buffer, g_input[0]);
      for (int i = 1; i<MAX_CLIENTS; i++)
      {
        strcat(buffer, ",");
        strcat(buffer, g_input[i]);
      }
      int size = strlen(buffer);
      buffer[size] = 255;
      buffer[0] = 0;
      for (int i = 0; i<g_numClients; i++) // broadcasting for now
      {
        send(g_client[i].s, buffer, size+1, 0);
      }
    }
  }
}

int _tmain(int argc, _TCHAR* argv[])
{
  SOCKET s = createBindAndListenSocket();
  if (s==0)
    return 0;
  for (int i = 0; i<MAX_CLIENTS; i++)
  {
    g_client[i].index = i;
    strcpy(g_input[i], "0");
  }

  // listening…  
  while (g_numClients < MAX_CLIENTS)
  {
    int addr_size = sizeof(g_client[g_numClients].address);
    g_client[g_numClients].s = accept(s, &g_client[g_numClients].address, &addr_size); 
    if (g_client[g_numClients].s == INVALID_SOCKET)
    { // error accepting connection
      return false;
    }
    else
    { // client connected successfully
      // start a thread that will communicate with client
      DWORD threadId;
      CreateThread(NULL, 0, runThread, &g_client[g_numClients], 0, &threadId);

      g_numClients++;

    }
  }


  shutdown(s, SD_BOTH);  // s cannot send anymore
  // you should check to see if any last data has arrived here
  closesocket(s);   // close


  WSACleanup();
  return 0;
}


#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "27016"
#define DEFAULT_BUFLEN 512

int receiveBuffer(SOCKET *ClientSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int bytesReceived, iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {
	    bytesReceived = recv(*ClientSocket, recvbuf, recvbuflen, 0);
	    if (bytesReceived > 0) {
	        printf("Bytes received: %d\n", bytesReceived);
	        // Echo the buffer back to the sender
	        iSendResult = send(*ClientSocket, recvbuf, bytesReceived, 0);
	        if (iSendResult == SOCKET_ERROR) {
	            printf("send failed: %d\n", WSAGetLastError());
	            return 1;
	        }
	        printf("Bytes sent: %d\n", iSendResult);
	    } else if (bytesReceived == 0)
	        printf("Connection closing...\n");
	    else {
	        printf("recv failed: %d\n", WSAGetLastError());	        
	        return 1;
	    }
	} while (bytesReceived > 0);

	// shutdown the connection since we're done
	bytesReceived = shutdown(*ClientSocket, SD_SEND);
	if (bytesReceived == SOCKET_ERROR) {
	    printf("shutdown failed with error: %d\n", WSAGetLastError());	      
	    return 1;
	}
	return 0;
}

SOCKET acceptSocket(SOCKET *ListenSocket)
{
	// Accept a client socket
    SOCKET ClientSocket = accept(*ListenSocket, NULL, NULL); 
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        //closesocket(ListenSocket);
        //WSACleanup();
        return INVALID_SOCKET;
    }
    return ClientSocket;
}

int listenSocket(SOCKET *ListenSocket, int backlog)
{
	if ( listen( *ListenSocket, backlog ) == SOCKET_ERROR ) {
	    printf( "Listen failed with error: %ld\n", WSAGetLastError() );
	    //closesocket(ListenSocket);	    
	    return SOCKET_ERROR;
	}
	return 0;
}

SOCKET createSocket(struct addrinfo *result)
{	
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
	    printf("Error at socket(): %ld\n", WSAGetLastError());
	    //freeaddrinfo(result);	    
    	return INVALID_SOCKET;
	}
	return ListenSocket;
}

int bindSocket(SOCKET *ListenSocket, struct addrinfo *result)
{
    int iResult = bind( *ListenSocket, result->ai_addr, (int)result->ai_addrlen);    
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        return SOCKET_ERROR;
    }
    return 0;
}

int initializeWinSock()
{
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;
	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }
}

int prepareSocket(SOCKET *ListenSocket, SOCKET *ClientSocket, struct addrinfo *result)
{
	initializeWinSock();

	struct addrinfo hints;

	int iResult;
	int err;

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		freeaddrinfo(result);
	    printf("getaddrinfo failed: %d\n", iResult);	    
	    return 1;
	}

	*ListenSocket = createSocket(result);
	//SOCKET ClientSocket = INVALID_SOCKET;

	err = bindSocket(ListenSocket, result);
	freeaddrinfo(result);
	if(err != 0) {
		return 1;
	}

	err = listenSocket(ListenSocket, SOMAXCONN);
	if(err != 0) {
		return 1;
	}

	*ClientSocket = acceptSocket(ListenSocket);	
	if(*ClientSocket == INVALID_SOCKET) {
		return 1;
	}

	err = receiveBuffer(ClientSocket);	
	if(err != 0) {
		return 1;
	}
}

int main(int argc, char const *argv[])
{
	struct addrinfo *result = NULL;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	int err = prepareSocket(&ListenSocket, &ClientSocket, result);

	WSACleanup();
	closesocket(ListenSocket);
	closesocket(ClientSocket);

	if(err != 0) {
		printf("There is an error, we're trying to solve it, goodbye\n");
		return 1;
	}

	return 0;
}

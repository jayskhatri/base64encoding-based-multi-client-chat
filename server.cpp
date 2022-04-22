#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <math.h>
using namespace std;
#define MXCHAR 500
#define __Z 0
#define CHRZR '0'
#define LISTENQ 10

//structure of the message
struct Msg {
	int type_id;
	char txt[MXCHAR];
};

// to store connected clients ip!!
char clientname[MXCHAR];

// store parents pid before forking.
pid_t parent_pid;
int Lfd, Cfd;

/** 
 this converts character array to a Msg structure
 **/
struct Msg* charToMsg(char* in){
	struct Msg* out = (struct Msg*) malloc(sizeof(struct Msg));
	strcpy(out->txt, in + 1);
	out->type_id = (int) (in[__Z] - CHRZR);
	return out;
}

/**
 concates the elements of Msg to a character array
 Format of message: [MESSAGE_TYPE_ID][MESSAGE_CONTENT]
 **/
char* msgToChar(struct Msg message){
	//creating the character array of size MXCHAR+1
	char *out = (char*) malloc((1+MXCHAR)*sizeof(char));
	out[__Z] = (char) ((int) CHRZR + message.type_id);
	strcpy(out+1, message.txt);
	return out;
}

/* 
character array to binary representation and storing this in integer 
array and returning to the caller also, this function will add 0-padding 
such that length is divisible by 6 

rlen: length of returned int array 
len: length of base64 encoded msg generated by encoding msg.
*/
int *charToBits(char *msg, int *rlen, int *len){
	int n = strlen(msg);
	const int six = 6;
	int size = n*8;
	if(size%six)
		size += (six -size%six);
	*len = size/six;
	if(n%3==1)
		(*len)+=2;
	if(n%3==2)
		(*len)+=1;
	*rlen = size;
	int *bits = (int *)malloc(size*sizeof(int));
	int idx = __Z;
	for(int i = __Z;i < n;i++){
		int p  = (int)msg[i];
		for(int j=__Z;j<8;j++){
			bits[idx + six + 1-j] = (p%2);
			p/=2;
		}
		idx += 8;
	}
	for(int i=idx;i<size;i++)
		bits[i] = __Z;
	return bits;
}


// returns corresponding char for each int in base64 encoding.
char base64Mapping(int i){
    if(i < 26)
        return 'A' + i;
    else if(i < 52)
        return 'a' + i -26;
    else if(i < 62)
        return CHRZR + i - 52;
    else
        return (i == 62)? '+':'/';
}


/**
 	bits - binary integer array
	s and e - index for knowing the ascii char represented by binary value from s to e
 **/
char binToChar(int *bits, int st, int end){
	int p = __Z,i;
	//evaluating decimal value from s to e binary value
	for(i=end;i>=st;i--)
		p = p + bits[i]*(1 << (end-i));
	return base64Mapping(p);
}


/**
 takes character array and encodes it with base64 encoding and modifies the input array with encoded data
**/
void encoding(char*data){
	int f;
	int encLen, bitsLen;
	char msg[MXCHAR];
	int n = strlen(data);
	strcpy(msg, data);

	//finding the \r or \n or \0
	for(int i=__Z;i<(int)strlen(data);i++){
		if(data[i] == '\r' || data[i] == '\n' || data[i] == '\0'){
			f = i;
			break;
		}
	}
	msg[f] = '\0';
	//converting char array to binary representation
	int *bits = charToBits(msg, &bitsLen, &encLen);
	//writing encoded data to the encoded_msg array
	char *encodedMsg = (char*) malloc((encLen+n-f+1)*sizeof(char));
	
	//converting binary to character array
	for(auto i=__Z;i<bitsLen/6;i++){
		encodedMsg[i] = binToChar(bits, i*6, i*6 + 5);
	}

	//adding =
	for(auto i=bitsLen/6;i<encLen;i++){
		encodedMsg[i] = '=';
	}
	//end of line
	encodedMsg[encLen] = '\0';
	//adding the rest of the data
	int p = encLen;
    for(int i=p;i<p+(n-f+1);i++){
		encodedMsg[i] = data[f + i-p];
	}
	
	strcpy(data, encodedMsg);
    free(encodedMsg);
	free(bits);
}


// converts decimal to binary numbers and returns the binary reprsentation in char array temp[i].
void decToBin(long n, int I, char out[][6]) {
    int r; //remainder
    int j = 5;
    for( ; n!=__Z; j--) {
        r = n%2;
        n = n/2;
		if(r == __Z){
			out[I][j] = CHRZR;
		}
		else{
			out[I][j] = '1';
		}
    }

    int i = __Z;
	while(i<=j) {
		out[I][i++] = CHRZR;
	}
}

//converting binary array to it's decimal representation
int binToDec(char *t ) {
    int decimal = __Z;
    //converting binary to decimal
	for(auto i = __Z; i<8; i++){
		if(t[7-i] == '1')
        	decimal +=(int) (1 << i);
	}
    return decimal;
}

/**
 * function to decode the msg, it actually makes copy msg which will have all of msg except '\n', '\0' and '\r'
 * now it will find the original msg by considering the = appended
 * translate the ciphered binary data into decimal format
 * in last it will append all the special characters
 **/
void decoding(char *msg){
	char cipheredMsg[100000];
	int f;
	//finding \n \r or \0
	for(int x=__Z;x<(int)strlen(msg);x++){
		if(msg[x] == '\n' || msg[x] == '\r' || msg[x] == '\0'){
			f = x;
			break;
		}
	}

	for(auto x=__Z;x<f;x++){
		cipheredMsg[x] = msg[x];
	}
	cipheredMsg[f] = '\0';
	
	//adding = to end
	int len = strlen(cipheredMsg);
	int sub = __Z;
	int aLength = len;
	if(cipheredMsg[len-1] == '='){
		if(cipheredMsg[len-2] == '='){
			aLength = len-2;
			sub = 4;
		}
		else{
			aLength = len-1;
			sub = 2;
		}
	}
	aLength = (aLength*6 - sub)/8;

	//base64 mapping
    char mapping[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	char temp[100000][6];
	char ftemp[600000];
	//converting ciphered msg to normal
	for(int i=__Z;i<(int)strlen(cipheredMsg);i++)
		for(int j=__Z;j<=63;j++)
			if(cipheredMsg[i]==mapping[j]){
				decToBin(j,i,temp);
				break;
			}	

	int tempS = strlen(temp[__Z]);
	int rmdr = strlen(cipheredMsg) * 6  - tempS;

	switch (rmdr){
		case 6:
			{
				for(int i=__Z;i<tempS-2;i++) // removed two __Z
					ftemp[i] = temp[__Z][i];
				break;	
			}
			
		case 12:
			{
				for(int i=__Z;i<tempS-4;i++)  
					ftemp[i] = temp[__Z][i];
				break;
			}


		default:
			{
				for(int i=__Z;i<tempS;i++)
					ftemp[i] = temp[__Z][i];
				break;
			}
	}

	char decipheredMessage[100000];
	int m = __Z;

	int flag = strlen(ftemp) / 8;
	int y = flag;
	
	while(flag){
		char x[8];
		for(int j=__Z;j<8;j++)
		    x[j] = ftemp[(y-flag)*8 + j];
		
		decipheredMessage[m++] = (char) binToDec(x);
		flag--;
	}

	decipheredMessage[aLength] = '\0';
	
	int p = strlen(decipheredMessage);
	int n = strlen(msg);
	for(int x=p;x<p+(n-f+1);x++)
		decipheredMessage[x] = msg[f + x-p];
	strcpy(msg, decipheredMessage);
	for(int i=__Z;i< 100000;i++){
		decipheredMessage[i] = '\0';
	}
	for(int i=__Z;i< 600000;i++){
		ftemp[i] = '\0';
	}
}

// writing n bytes to descriptor
ssize_t written(int fd, const void *vptr, size_t n) {
    size_t leftn;
    ssize_t nwritten;
    const char *ptr;

    ptr = (char *)vptr;
    leftn = n;
    for ( ; leftn > __Z; ) {
        if ((nwritten = write(fd, ptr, leftn)) <= __Z) {
            if (errno == EINTR)
				// call write() again 
                nwritten = __Z;           
            else // error
                return(-1);             
        }

        leftn -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}

// returns true if s is an integer
bool isInteger(char *s){
	auto i=0;
	while(i<(int)strlen(s)){
		if(s[i] < CHRZR || s[i] > '9')
			return false;

		i++;
	}
	
	return true;
}

// handles errors
void Written(int file_d, void *pointer, size_t NB){
    if ((int)written(file_d, pointer, NB) != NB)
        perror("written error");
}

// returns true if M starts with "exit"
bool isExit(char *M){
	if(strlen(M) < 4 || M[__Z] != 'e' || M[1] != 'x'|| M[2] != 'i'||M[3] != 't')
		return false;
	return true;
}

/**
 - waits for message of client
 - receives the message from client and creates the "Msg" object(message) from the received character array(buf)
 - displays client ID, MESSAGE_ID
 - decodes the content the "Msg" object
 - creates another "Msg" object(smessage) corresponding to "ACK" and store its message type ID and
   content into smessage object
 - translates the message object into character array and sends it to client
**/
void boot_server_run(int sockfd){
	ssize_t n;
	char buf[MXCHAR];
	bzero(buf, MXCHAR*sizeof(char));
	loop:
		// wait for message from client
		while((n = read(sockfd, buf, MXCHAR)) > __Z){
			struct Msg* message = charToMsg(buf);
			
			cout<<"Msg received from client "<<clientname<<endl;
            cout<<"Type id:"<<message->type_id<<" || message: "<< message->txt<<endl;
			cout<<"Encoded message: "<<message->txt<<endl;
			decoding(message->txt);
			//storing true if it was termination message
			bool is_close = isExit(message->txt);
			cout<<"Decoded message: "<<message->txt<<endl;
			free(message);
			
			// making ACK message to send to client
			struct Msg smessage;
			smessage.type_id = 2;
			snprintf(smessage.txt, sizeof(smessage.txt),"ACK\r\n");
			encoding(smessage.txt);
			//converting message to char array
			char *message_com = msgToChar(smessage);
			// sending ACK message to client
			Written(sockfd, message_com, strlen(message_com));
			free(message_com);
			bzero(buf, sizeof(buf));
				
			//checking if it was connection termination
			if(is_close){
				cout<<"-client wants to close.\n";
				cout<<"-closing the client socket\n";
				close(sockfd);
				exit(1);
			}
		}
		if(n < __Z){
			goto loop;
		}
}


/**
 * sending close signal to client to terminate connection
 * this will release sockets and close the connection
**/
void handleSignals(int sV){
	cout<<"sending close to clients...\n";
	char sendline[MXCHAR];
	struct Msg message;
	message.type_id = 3;
	//setting setline array value to __Z
	bzero(sendline, sizeof(sendline));
	snprintf(sendline, sizeof(sendline), "exit\r\n");
	encoding(sendline);
	strcpy(message.txt, sendline);
	char* message_com = msgToChar(message);
	// send close to all clients
	if(getpid() != parent_pid)
		Written(Cfd, message_com, strlen(message_com));
	free(message_com);

	// pid is of parent process
	if(getpid() == parent_pid)
		cout<<"closing sockets....\n";
	close(Lfd);

	if(getpid() == parent_pid)
		cout<<"exiting...\n";
	exit(__Z);
}

//driver program
int main(int argc, char **argv){
	parent_pid = getpid();
	//signalling all the processes to release the sockets
	signal(SIGINT, handleSignals);

	//checking for the valid arguments
	if(argc != 2){
		//checking wherher the passed integer is unsigned int
		if(!isInteger(argv[1])){
			cout<<"port number must be unsigned int\n";
			exit(1);
		}
		cout<<"please check arguments: <executable> <port>\n";
		exit(1);
	}

	//storing port number
	int serv_port = atoi(argv[1]);

	//condition to check the port number
	if(serv_port < __Z || serv_port > 65535){
		cout<<"port number must be between 0 and 65535.\n";
		exit(1);
	}

	//needed variables
	int lFileD, connectionFD;
	pid_t child_pid;
	socklen_t cli_len;
	struct sockaddr_in cli_addr, serv_addr;

	//creating socket
	//socket() function will return unique descriptor
	lFileD = socket(AF_INET, SOCK_STREAM, __Z);
	cout<<"socket created!\n";
	Lfd = lFileD;
	//setting up server address
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serv_port);

	//bind socket to port
	bind(lFileD, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	//listen for connections
	listen(lFileD, LISTENQ);
	cout<<"listening at port: "<<serv_port<<endl;
	
	//accepting connections
	while(1){
		//accepting connections
		cli_len = sizeof(cli_addr);
		connectionFD = accept(lFileD, (struct sockaddr *) &cli_addr, &cli_len);
		char client[100];
		//converting binary internet address to string address
		inet_ntop(AF_INET, &cli_addr.sin_addr, client, 100);
		cout<<client<< " client connected\n";
		strcpy(clientname, client);

		//forking the process
		if((child_pid = fork()) == __Z){
			Cfd = connectionFD;
			close(lFileD);
			// start socket and wait for client requests
			boot_server_run(connectionFD);
			exit(__Z);
		}

		//parent process
		if(getpid() == parent_pid)
			close(connectionFD);
	}
	return 0;
}

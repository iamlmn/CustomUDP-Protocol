/* Name: Lakshmi Naarayanan
Student_Id:00001609965*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <stdint.h>


#define PORT 30000


//Primitives
#define PACKETID 0XFFFF
#define CLIENTID 0XFF
#define ENDPACKETID 0XFFFF

//server responses
#define PAID 0XFFFB
#define NOTPAID 0XFFF9
#define NOTEXIST 0XFFFA

// Technologies definition
#define TECHNOLOGY_2G    (02)
#define TECHNOLOGY_3G    (03)
#define TECHNOLOGY_4G    (04)
#define TECHNOLOGY_5G    (05)

//Retry count
#define MAXRETRY 3
#define MAXREQUESTTIMEOUTSECONDS 3
#define MAXREQUESTTIMEOUTMICROSECONDS 0
//Structure defining Access permission request packet 
struct requestPacket {
	
	uint16_t packet_id;
	uint8_t client_id;
	uint16_t acc_per;
	uint8_t segment_no;
	uint8_t length;
	uint8_t technology;
	unsigned int source_subscriber_no;
	uint16_t end_packet_id;
};

// Structure defining Response packet
struct responsePacket {
	
	uint16_t packet_id;
	uint8_t client_id;
	uint16_t type;
	uint8_t segment_no;
	uint8_t length;
	uint8_t technology;
	unsigned int source_subscriber_no;
	uint16_t endpacket_id;
};


// Printing all packet details.
void printPacketDetails(struct requestPacket request) {
	printf("Packet Info \n");
	printf("\t Packet ID: %x\n",request.packet_id);
	printf("\t Client ID : %x\n",request.packet_id);
	printf("\t Access permission: %x\n",request.acc_per);
	printf("\t Segment No : %d \n",request.segment_no);
	printf("\t Length %d\n",request.length);
	printf("\t Technology %d \n", request.technology);
	printf("\t Subscriber No: %u \n",request.source_subscriber_no);
	printf("\t End of Datapacket id : %x \n",request.end_packet_id);
}

//method to declare requestPacket with data
struct requestPacket declareRequestPacket() {
	
	struct requestPacket request;
	request.packet_id = PACKETID;
	request.packet_id = CLIENTID;
	request.acc_per = 0XFFF8;
	request.end_packet_id = ENDPACKETID;
	
	return request;
}


int main(int argc, char**argv){
	
	struct requestPacket request;
	struct responsePacket response;
	char line[30];
	FILE *file_pointer;
	int sockfd,n = 0;
	struct sockaddr_in cliaddr;
	socklen_t addr_size;
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	int segment_no_counter = 1;
	
	//socket timeout
	struct timeval timer;
	timer.tv_sec = MAXREQUESTTIMEOUTSECONDS;
	timer.tv_usec = MAXREQUESTTIMEOUTMICROSECONDS;	
	
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timer,sizeof(struct timeval));
	int retryCounter = 0;
	if(sockfd < 0) {
		printf("\n ERROR: Unable to Create Socket! \n");
	}
	//UDP protocol binding
	bzero(&cliaddr,sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cliaddr.sin_port=htons(PORT);
	addr_size = sizeof cliaddr;
	request = declareRequestPacket();

	//get input data from txt file
	file_pointer = fopen("SampleInput-PA2.txt", "rt");
	if(file_pointer == NULL)
	{
		printf("\n ERROR: File not found \n");
	}
	
	while(fgets(line, sizeof(line), file_pointer) != NULL) {
		
		n = 0;
		retryCounter = 0;
		printf("\n");
		
		char * words;
		words = strtok(line," "); //string to token
		request.length = strlen(words);
		request.source_subscriber_no = (unsigned) atoi(words);
		words = strtok(NULL," ");
		request.length += strlen(words);
		request.technology = atoi(words);
		words = strtok(NULL," ");
		request.segment_no = segment_no_counter;
		
		printPacketDetails(request);
		while(n <= 0 && retryCounter < MAXRETRY) {
			
			//send and recieve packets
			sendto(sockfd,&request,sizeof(struct requestPacket),0,(struct sockaddr *)&cliaddr,addr_size);
			n = recvfrom(sockfd,&response,sizeof(struct responsePacket),0,NULL,NULL);
			
			if(n <= 0 ) {
				printf("\n ERROR: No response from server\n");
				printf("Sending packet again \n");
				retryCounter++;
			}
			
			else if(n > 0) {
				// printf("\n : \n");
				if(response.type == NOTPAID) {
					printf("INFO: Subscriber %d has not paid  \n",request.source_subscriber_no);
				}
				else if(response.type == NOTEXIST ) {
					printf("INFO:  Subscriber %d number is not found\n",request.source_subscriber_no );
				}
				else if(response.type == PAID) {
					 printf("INFO:  Subscriber %d permitted to access \n", request.source_subscriber_no);

				}
			}
		}
		
		//If no ACK recieved after sending packet after maximum number of retries
		if(retryCounter>= MAXRETRY ) {
			printf("\n ERROR: Server is not responding ....\n");
			exit(0);
		}
		printf("\n ********************************************************** \n");
		segment_no_counter++;
	}
	fclose(file_pointer);
}

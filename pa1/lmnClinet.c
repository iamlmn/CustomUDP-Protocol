#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define PORT 32033
// #define TIMEOUT 3

// Default givens
#define START_PACK_ID 0XFFFF // 
#define END_PACK_ID 0XFFFF
#define CLIENT_ID 0XFF
#define CLIENT_ID_MAX 255
#define DATA_LEN_MAX 255  

//Packet Types
#define DATA 0XFFF1
#define ACK 0XFFF2
#define REJECT 0XFFF3

//Reject sub codes
#define REJ_OUT_SEQ 0XFFF4
#define REJ_LEN_MISMATCH 0XFFF5
#define REJ_END_MISS 0XFFF6
#define REJ_DUP_PACK 0XFFF7


// Timeout values
#define TIMEOUT_SECS    3       /* number of seconds between retransmits */
#define MAXTRIES        3       /* number of tries before giving up */


// Segment to simulate error
#define DATALENGTHMISMATCHSEGMENT 8
#define OUTOFSEQUENCESEGMENT 10
#define DUPLICATEPACKETSEGMENT 7
#define ENDOFPACKETMISSINGSEGMENT 9
#define MAXRETRIES 3

//Data Packet Format
struct dataPacket{
	
	uint16_t start_packet_id;
	uint8_t client_id;
	uint16_t type;
	uint8_t segment_no;
	uint8_t length;
	char payload[255];
	uint16_t endpacket_id;
};


// // ACK (Acknowledge) Packet Format

// struct ackPacket{
//     uint16_t start_packet_id;
//     uint8_t client_id;
//     uint16_t type;
//     uint8_t recieved_segment_no;
// 	uint16_t endpacket_id;
// }

//REJECT Packet Format
struct rejectPacket {
	
	uint16_t start_packet_id;
	uint8_t client_id;
	uint16_t type;
	uint16_t reject_subcode;
	uint8_t recieved_segment_no;
	uint16_t endpacket_id;
};



//declare dataPacket with given data
struct dataPacket declareDataPacket() {
	
	struct dataPacket data;
	data.packet_ID = START_PACK_ID;
	data.client_ID = CLIENT_ID;
	data.type = DATA;
	data.endpacket_ID = ENDPACKET_ID;
	
	return data;
}


//print all the packet details
void printPacketDetails(struct dataPacket data) {
			
	printf("\n INFO: Client Sending packet:\n");
	printf("  Packet ID: %x\n",data.start_packet_id);
	printf("  Client ID : %hhx\n",data.client_id);
	printf("  Data: %x\n",data.type);
	printf("  Segment nNo : %d \n",data.segment_no);
	printf("  Length %d\n",data.length);
	printf("  Payload: %s\n",data.payload);
	printf("  End of Packet ID : %x\n",data.endpacket_id);
}


int main(){
	printf("Hi");
	struct dataPacket data;
	struct rejectPacket receivedpacket;
	struct sockaddr_in cliaddr;
	socklen_t addr_size;
	FILE *filePointer;
	char line[255];
	int sockfd;
	int n = 0;
	int retryCounter = 0;
	int segmentNo = 1;


    sockfd = socket(AF_INET,SOCK_DGRAM,0); //socket(domain, type, protocol), UDP - DGRAM
	if(sockfd < 0) {
		printf("\n ERROR: Socket Failure \n");
	}

    //creation of UDP socket
	bzero(&cliaddr,sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long address
	cliaddr.sin_port=htons(PORT); //host to network short address
	addr_size = sizeof cliaddr ;

    //socket timeout
	struct timeval timer;
	timer.tv_sec = TIMEOUT_SECS;
	timer.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timer,sizeof(struct timeval));
	data = declareDataPacket();

    //get payload data from txt file
	filePointer = fopen("SampleInput-PA1.txt", "rt");
	if(filePointer == NULL)
	{
		printf("\n ERROR: File not found \n");
		exit(0);
	}



	while(fgets(line, sizeof(line), filePointer) != NULL) {	
		
		n = 0;
		retryCounter = 0;
		printf("\n");
		data.segment_No = segmentNo;
		strcpy(data.payload,line);
		data.length = strlen(data.payload);
		//create length mismatch packet
		if(segmentNo == DATALENGTHMISMATCHSEGMENT) {
			data.length++;
		}
		
		//create out of sequence error packet
		if(segmentNo == OUTOFSEQUENCESEGMENT) {
			data.segment_no = data.segment_no + 12; // random number
		}
		
		//create duplicate packet
		if(segmentNo == DUPLICATEPACKETSEGMENT) {
			data.segment_no = 3;   // At 7 creating segment 3 again
		}
		
		//create enf of packet missing error packet
		if(segmentNo == ENDOFPACKETMISSINGSEGMENT) {
			data.endpacket_id= 0;
		}
		
		
		if(segmentNo != ENDOFPACKETMISSINGSEGMENT) {
			data.endpacket_id = ENDPACKET_ID; // Fixing end of packet missing for future packets.
		}

		printPacketDetails(data);
		while(n <= 0 && retryCounter < MAXRETRIES) {
			
			//send and receive packets
			sendto(sockfd,&data,sizeof(struct dataPacket),0,(struct sockaddr *)&cliaddr,addr_size);
			n = recvfrom(sockfd,&receivedpacket,sizeof(struct rejectPacket),0,NULL,NULL);

			if(n <= 0 ) {
				printf("\n ERROR: No response from server\n");
				printf("Sending packet again \n");
				retryCounter++;
			}
			
			else if(receivedpacket.type == ACKPACKET  ) {
				printf("\n INFO: ACK packet received \n");
			}
			
			else if(receivedpacket.type == REJECTPACKET ) {
				printf("\n ERROR: REJECT received \n");
				printf("type : %x \n" , receivedpacket.subcode);
				if(receivedpacket.subcode == LENGTHMISMATCHCODE ) {
					printf("Length Mismatch \n");
				}
				else if(receivedpacket.subcode == ENDOFPACKETMISSINGCODE ) {
					printf("End of Packet Missing \n");
				}
				else if(receivedpacket.subcode == OUTOFSEQUENCECODE ) {
					printf("Out of Sequence \n");
				}
				else if(receivedpacket.subcode == DUPLICATECODE) {
					printf("Duplicate Packet \n");
				}
			}
		}
		
		//no ACK received after sending packet 3 times
		if(retryCounter>= MAXRETRIES ) {
			printf("\n ERROR: Server does not respond \n");
			exit(0);
		}
		segmentNo++;
		printf("\n ************************************************************** \n");
	}
	fclose(filePointer);
}



/* Name: Lakshmi Naarayanan
Student_Id:00001609965*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define PORT 30000
#define ENTRY 10

//response messages
#define PAID 0XFFFB
#define NOTPAID 0XFFF9
#define NOTEXIST 0XFFFA

// #define TECHNOLOGY_2G    (02)
// #define TECHNOLOGY_3G    (03)
// #define TECHNOLOGY_4G    (04)
// #define TECHNOLOGY_5G    (05)

//Structure defining Request packet format
struct requestPacket{
	
	uint16_t packet_id;
	uint8_t client_id;
	uint16_t acc_permission;
	uint8_t segment_no_counter;
	uint8_t length;
	uint8_t technology;
	unsigned int source_subscriber_no;
	uint16_t end_packet_id;
};

// Structure defining Response packet format
struct responsePacket {
	uint16_t packet_id;
	uint8_t client_id;
	uint16_t type;
	uint8_t segment_no;
	uint8_t length;
	uint8_t technology;
	unsigned int source_subscriber_no;
	uint16_t end_packet_id;
};

// function to create a packet for response
struct responsePacket createResponsePacket(struct requestPacket request) {
	
	struct responsePacket response;
	
	response.packet_id = request.packet_id;
	response.client_id = request.client_id;
	response.segment_no = request.segment_no_counter;
	response.length = request.length;
	response.technology = request.technology;
	response.source_subscriber_no = request.source_subscriber_no;
	response.end_packet_id = request.end_packet_id;

	return response;
}


// create a Map to store subscriber to technology with status
struct Map {
	
	unsigned long subscriberNumber;
	uint8_t technology;
	int status;
};



void getSubscriberData(struct Map map[]) {

	//save the file on server
	char line[30];
	int segment = 0;
	FILE *file_pointer;

	file_pointer = fopen("Verification_Database.txt", "rt"); //read text from file
	
	if(file_pointer == NULL)
	{
		printf("\n ERROR: File not found \n");
		return;
	}
	
	while(fgets(line, sizeof(line), file_pointer) != NULL)
	{
		char * words;

		words = strtok(line," ");

		map[segment].subscriberNumber =(unsigned) atol(words);
		words = strtok(NULL," ");

		map[segment].technology = atoi(words);
		words = strtok(NULL," ");

		map[segment].status = atoi(words);
		segment++;
	}
	fclose(file_pointer);
}


// Verifies status of subscriber
int verifySubsriber(struct Map map[],unsigned int subscriberNumber,uint8_t technology) {
	int value = -1;
	for(int j = 0; j < ENTRY;j++) {
		if(map[j].subscriberNumber == subscriberNumber && map[j].technology == technology) {
			return map[j].status;
		}
	}
	return value;
}


// print all the packet details
void displayPacketDetails(struct requestPacket request) {
	
	printf(" Packet ID: %x\n",request.packet_id);
	printf(" Client Id : %hhx\n",request.client_id);
	printf(" Access permission: %x\n",request.acc_permission);
	printf(" Segment no : %d \n",request.segment_no_counter);
	printf(" Length %d\n",request.length);
	printf(" Technology %d \n", request.technology);
	printf(" Subscriber no: %u \n",request.source_subscriber_no);
	printf(" End of request packet id : %x \n",request.end_packet_id);
}


int main(int argc, char**argv){
	
	struct requestPacket request;
	struct responsePacket response;
	struct Map map[ENTRY];
	getSubscriberData(map);
	int sockfd,rv;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	bzero(&serverAddr,sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	serverAddr.sin_port=htons(PORT);
	bind(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
	addr_size = sizeof serverAddr;
	printf("\n Server started successfully \n");
	
	while(1) {
		
		//wait and recieve client packet
		rv = recvfrom(sockfd,&request,sizeof(struct requestPacket),0,(struct sockaddr *)&serverStorage, &addr_size);
		
		//print the recieved packet details
		displayPacketDetails(request);
		
		//to check for ack_timer
		/*if(request.segment_No == 9) {
			exit(0);
		}*/

		if(rv > 0 && request.acc_permission == 0XFFF8) {
			
			response = createResponsePacket(request);

			int value = verifySubsriber(map,request.source_subscriber_no,request.technology);
			
			//subscriber has not paid
			if(value == 0) {
				response.type = NOTPAID;
				printf("\n INFO: Subscriber has not paid \n");
			}
			
			//subscriber does not exist on database
			else if(value == -1) {
				printf("\n INFO: Subscriber does not exist on database \n");
				response.type = NOTEXIST;
			}
			
			//subscriber permitted to acces the network
			else if(value == 1) {
				printf("\n INFO: Subscriber permitted to access the network \n");
				response.type = PAID;
			}
			
			//send response packet
			sendto(sockfd,&response,sizeof(struct responsePacket),0,(struct sockaddr *)&serverStorage,addr_size);
		}
		rv = 0; // reset rv
		printf("\n *************************************************************** \n");
	}
}

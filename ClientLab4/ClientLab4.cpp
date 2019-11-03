// ClientLab4.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h> 
#include <WS2tcpip.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <sstream>
#include <vector>
#pragma comment(lib, "ws2_32.lib")


constexpr auto MAXNAMELEN = 32;

enum ObjectDesc
{
	Human,
	NonHuman,
	Vehicle,
	StaticObject
};

enum ObjectForm
{
	Cube,
	Sphere,
	Pyramid,
	Cone
};

struct Coordinate
{
	int x;
	int y;
};



// message head
enum MsgType
{
	Join,
	Leave,
	Change,
	Event,
	TextMessage


};

struct MsgHead
{
	unsigned int length;
	unsigned int seq_num;
	unsigned int id;
	MsgType type;
};


//message type Join
struct JoinMsg
{
	MsgHead head;
	ObjectDesc desc;
	ObjectForm form;
	char name[MAXNAMELEN];


};



//message type leave
struct LeaveMsg
{
	MsgHead head;

};



//message type change
enum ChangeType
{
	NewPlayer,
	PlayerLeave,
	NewPlayerPosition
};

struct ChangeMsg
{
	MsgHead head;
	ChangeType type;
};



//variations of ChangeMsg
struct NewPlayerMsg
{
	ChangeMsg msg;
	ObjectDesc desc;
	ObjectForm form;
	char name[MAXNAMELEN];

};

struct PlayerLeaveMsg
{
	ChangeMsg msg;
};

struct NewPlayerPositionMsg
{
	ChangeMsg msg;
	Coordinate pos;
	Coordinate dir;

};


//Message of type event
enum EventType
{
	Move

};

struct EventMsg
{
	MsgHead head;
	EventType type;


};


//variations of EventMsg

struct MoveEvent
{
	EventMsg event;
	Coordinate pos;
	Coordinate dir;

};



//messages of type TextMessage
struct TextMessageMsg
{
	MsgHead head;
	char text[1];


};

void main()
{

	int port = 49152;
	std::string ipAddress = "130.240.74.17";
	int field[201][201];

	//initialize the array to all zeros
	for (int i = 0; i < 201; i++) {
		for (int j = 0; j < 201; j++) {
			field[i][j] = 0;
		}

	}
	
	//initialize winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Can't start winsock, err #" << wsResult << std::endl;
		return;

	}



	//create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Can't create socket, err #" << WSAGetLastError << std::endl;
		WSACleanup();
		return;

	}



	//fill in a hint structure
	sockaddr_in hint1;
	hint1.sin_family = AF_INET;
	hint1.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint1.sin_addr);


	std::string local = "::1";
	sockaddr_in6 hint2;
	hint2.sin6_family = AF_INET6;
	hint2.sin6_port = htons(5000);
	hint2.sin6_flowinfo = 0;
	hint2.sin6_scope_id = 0;
	inet_pton(AF_INET6, local.c_str(), &hint2.sin6_addr);

	
	//create udp socket
	SOCKET UDPSock = socket(AF_INET6, SOCK_DGRAM, 0);


	//connect to server
	int connResultTCP = connect(sock, (sockaddr*)&hint1, sizeof(hint1));
	if (connResultTCP == SOCKET_ERROR)
	{
		std::cerr << "Can't connect to server, Err #" << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return;
	}



	char buf[4096];
	std::string userInput;

	int id{};
	int seq_num{};

	do {
		try {
			std::cout << "::> ";
			std::cin >> userInput;

			std::stringstream input(userInput);
			std::string segment;
			std::vector<std::string> segList;

			while (std::getline(input, segment, ',')) {
				segList.push_back(segment);
			}

			std::string msgtype = segList.at(0);
			int b = std::stoi(msgtype);

			

			switch (b) {

			//join message
			case 0:
				JoinMsg joinmsg;

				joinmsg.head.id = 0;
				joinmsg.head.seq_num = 0;
				joinmsg.head.length = sizeof(joinmsg);
				joinmsg.head.type = Join;

				strcpy_s(joinmsg.name, "Johan");

				char joinsendbuf[sizeof(joinmsg)];

				memcpy((void*)joinsendbuf, (void*)&joinmsg, sizeof(joinmsg));

				send(sock, joinsendbuf, sizeof(joinsendbuf), 0);

				break;

			//leave message
			case 1: 

				LeaveMsg leavemsg;

				leavemsg.head.type = Leave;
				leavemsg.head.id = id;
				leavemsg.head.seq_num = seq_num;
				leavemsg.head.length = sizeof(leavemsg);

				char leavesendbuf[sizeof(leavemsg)];

				memcpy((void*)leavesendbuf, (void*)&leavemsg, sizeof(leavemsg));

				send(sock, leavesendbuf, sizeof(leavemsg), 0);


				break;


			//move message
			case 2:
				MoveEvent moveevnt;

				moveevnt.pos.x = std::stoi(segList.at(1));
				moveevnt.pos.y = std::stoi(segList.at(2));

				//moveevnt.dir.x = std::stoi(segList.at(1));
				//moveevnt.dir.y = std::stoi(segList.at(2));

				moveevnt.event.type = Move;

				moveevnt.event.head.type = Event;
				moveevnt.event.head.id = id;
				moveevnt.event.head.seq_num = seq_num;
				moveevnt.event.head.length = sizeof(moveevnt);


				char eventsendbuf[sizeof(moveevnt)];

				memcpy((void*)eventsendbuf, (void*)&moveevnt, sizeof(moveevnt));

				send(sock, eventsendbuf, sizeof(eventsendbuf), 0);

				break;




			default:
				std::cout << "No command available\n";
				break;
			}


			

			//receive response
			recv(sock, buf, 4096, 0);

			MsgHead* msghead;
			ChangeMsg* changemsg;



			changemsg = (ChangeMsg*)buf;
			msghead = (MsgHead*)buf;

			id = msghead->id;
			seq_num = msghead->seq_num;

			std::cout << msghead->type;

			
			//the response can either be a join-message(0), or change-message(2)
			switch (msghead->type) {

				//join message
			case 0: {
				//need to read the duplicate buffer from the test server
				recv(sock, buf, 4096, 0);
				

			   field[0][0] = msghead->id;
				  std::string o = std::to_string(0) + "," + std::to_string(0) + ",blue";
				  sendto(UDPSock, o.c_str(), o.size() + 1, 0, (sockaddr*)&hint2, sizeof(hint2));
				  
				  break;

			}

			//change message, something has changed and we need to update the gui
			case 2: {
				std::cout << changemsg->type;
				//find a free space to spawn in
				if (changemsg->type == 0) {
					bool flag = true;
					for (int i = 0; i < 201; i++) {
						if (flag) {
							for (int j = 0; j < 201; j++) {
								if (field[i][j] == 0) {
									field[i][j] = msghead->id;
									std::cout << "field[" << i << "][" << j << "] = " << msghead->id << std::endl;
									std::string p = std::to_string(i) + "," + std::to_string(j) + ",blue";
									sendto(UDPSock, p.c_str(), p.size() + 1, 0, (sockaddr*)&hint2, sizeof(hint2));
									flag = false;
									break;
								}
							}

						}

					}
					std::cout << "New player\n";


				}
				else if (changemsg->type == 1) {
					PlayerLeaveMsg* plm;
					plm = (PlayerLeaveMsg*)buf;
					int playerid = plm->msg.head.id;

					bool flag = true;
					for (int i = 0; i < 201; i++) {
						if (flag) {
							for (int j = 0; j < 201; j++) {
								if (field[i][j] == playerid) {
									std::cout << "player found, ID = " << playerid << std::endl;
									field[i][j] = 0;
									std::string p = std::to_string(i) + "," + std::to_string(j) + ",white";
									sendto(UDPSock, p.c_str(), p.size() + 1, 0, (sockaddr*)&hint2, sizeof(hint2));
									flag = false;
									break;

								}
							}
						}
					}


					std::cout << "Player left\n";

				}
				else if (changemsg->type == 2) {

					NewPlayerPositionMsg* nppm;
					nppm = (NewPlayerPositionMsg*)buf;
					int playerid = nppm->msg.head.id;

					bool flag = true;
					for (int i = 0; i < 201; i++) {
						if (flag) {
							for (int j = 0; j < 201; j++) {
								if (field[i][j] == playerid) {
									std::cout << "player found, ID = " << playerid << std::endl;
									field[i][j] = 0;
									std::string p = std::to_string(i) + "," + std::to_string(j) + ",white";
									sendto(UDPSock, p.c_str(), p.size() + 1, 0, (sockaddr*)&hint2, sizeof(hint2));
									flag = false;
									break;

								}
							}
						}

					}

					field[nppm->pos.x + 100][nppm->pos.y + 100] = playerid;
					std::string xPos = std::to_string(nppm->pos.x + 100);
					std::string yPos = std::to_string(nppm->pos.y + 100);


					std::cout << "Player moved\n";
					std::string q = xPos + "," + yPos + ",red";



					sendto(UDPSock, q.c_str(), q.size() + 1, 0, (sockaddr*)&hint2, sizeof(hint2));


				}


				break;
			}

			}


		}
		catch (std::exception e) {
			std::cout << "Something went wrong\n";
		}

	} while (userInput.size() > 0);






};


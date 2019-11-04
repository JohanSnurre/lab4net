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
#include <thread>
#include <conio.h>
#pragma comment(lib, "ws2_32.lib")


constexpr auto MAXNAMELEN = 32;


int field[201][201];
std::string serverIpAddress = "130.240.74.14";
int serverPort;
int playerID;






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





int getInput() {
	char key = _getch();
	int value = key;

	switch (key) {
	case 119:
		return 0;
		break;

	case 97:
		return 1;
		break;

	case 115:
		return 2;
		break;

	case 100:
		return 3;
		break;


	case 27:
		return 4;
		break;
	}


}



void sendMoveRequest(SOCKET sock, int currX, int currY, int deltaX, int deltaY, int id, int seq_num) {

	MoveEvent moveevnt;
	moveevnt.pos.x = currX + deltaX - 100;
	moveevnt.pos.y = currY + deltaY - 100;



	moveevnt.event.type = Move;

	moveevnt.event.head.type = Event;
	moveevnt.event.head.id = id;
	moveevnt.event.head.seq_num = seq_num;
	moveevnt.event.head.length = sizeof(moveevnt);


	std::cout << sizeof(moveevnt) << std::endl;
	char eventsendbuf[sizeof(moveevnt)];

	memcpy((void*)eventsendbuf, (void*)&moveevnt, sizeof(moveevnt));
	send(sock, eventsendbuf, sizeof(eventsendbuf), 0);


	return;

}



void sending(SOCKET sock) {
	int seq_num = 0;

	JoinMsg joinmsg;

	joinmsg.head.id = 0;
	joinmsg.head.seq_num = 0;
	joinmsg.head.length = sizeof(joinmsg);
	joinmsg.head.type = Join;

	strcpy_s(joinmsg.name, "Johan");

	char joinsendbuf[sizeof(joinmsg)];

	memcpy((void*)joinsendbuf, (void*)&joinmsg, sizeof(joinmsg));

	send(sock, joinsendbuf, sizeof(joinsendbuf), 0);

	seq_num = seq_num + 1;




	while (true) {
		int input = getInput();


		std::cout << input;
		//leave message
		if (input == 4) {
			LeaveMsg leavemsg;

			leavemsg.head.type = Leave;
			leavemsg.head.id = playerID;
			leavemsg.head.seq_num = seq_num;
			leavemsg.head.length = sizeof(leavemsg);

			char leavesendbuf[sizeof(leavemsg)];

			memcpy((void*)leavesendbuf, (void*)&leavemsg, sizeof(leavemsg));

			send(sock, leavesendbuf, sizeof(leavemsg), 0);

			std::cout << "Leaving\n";

			break;

		}

		int currX{};
		int currY{};
		int deltaX{};
		int deltaY{};





		if (input == 0) {
			deltaX = 0;
			deltaY = -1;
		}
		else if (input == 1) {
			deltaX = -1;
			deltaY = 0;
		}
		else if (input == 2) {
			deltaX = 0;
			deltaY = 1;
		}
		else if (input == 3) {
			deltaX = 1;
			deltaY = 0;
		}




		bool found = false;
		for (int x = 0; x < 201; x++) {
			if (!found) {
				for (int y = 0; y < 201; y++) {
					if (field[x][y] == playerID) {
						found = true;
						currX = x;
						currY = y;
						break;

					}

				}


			}

		}


		MoveEvent moveevnt;
		moveevnt.pos.x = currX + deltaX - 100;
		moveevnt.pos.y = currY + deltaY - 100;



		moveevnt.event.type = Move;

		moveevnt.event.head.type = Event;
		moveevnt.event.head.id = playerID;
		moveevnt.event.head.seq_num = seq_num;
		moveevnt.event.head.length = sizeof(moveevnt);


		std::cout << sizeof(moveevnt) << std::endl;
		char eventsendbuf[sizeof(moveevnt)];

		memcpy((void*)eventsendbuf, (void*)&moveevnt, sizeof(moveevnt));
		send(sock, eventsendbuf, sizeof(eventsendbuf), 0);
		//sendMoveRequest(sock,currX,currY,deltaX,deltaY,playerID,seq_num);
		seq_num = seq_num + 1;


	}

	return;
}




void receiving(SOCKET sock, SOCKET UDPSock, sockaddr_in6 UDPHint) {




	char buf[4096];

	int seq_num;


	while (true) {
		recv(sock, buf, 4096, 0);




		//decode the message

		MsgHead* msghead;
		ChangeMsg* changemsg;



		changemsg = (ChangeMsg*)buf;
		msghead = (MsgHead*)buf;

		playerID = msghead->id;
		seq_num = msghead->seq_num;




		//the response can either be a join-message(0), or change-message(2)
		switch (msghead->type) {

			//join message
		case 0: {
			//need to read the duplicate buffer from the test server
			recv(sock, buf, 4096, 0);


			field[0][0] = msghead->id;
			std::string o = std::to_string(0) + "," + std::to_string(0) + ",blue";
			sendto(UDPSock, o.c_str(), o.size() + 1, 0, (sockaddr*)&UDPHint, sizeof(UDPHint));

			break;

		}

			  //change message, something has changed and we need to update the gui
		case 2: {
			//find a free space to spawn in
			if (changemsg->type == 0) {
				bool flag = true;
				for (int i = 0; i < 201; i++) {
					if (flag) {
						for (int j = 0; j < 201; j++) {
							if (field[i][j] == 0) {
								field[i][j] = msghead->id;
								//std::cout << "field[" << i << "][" << j << "] = " << msghead->id << std::endl;
								std::string p = std::to_string(i) + "," + std::to_string(j) + ",blue";
								sendto(UDPSock, p.c_str(), p.size() + 1, 0, (sockaddr*)&UDPHint, sizeof(UDPHint));
								flag = false;
								break;
							}
						}

					}

				}


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
								sendto(UDPSock, p.c_str(), p.size() + 1, 0, (sockaddr*)&UDPHint, sizeof(UDPHint));
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
								sendto(UDPSock, p.c_str(), p.size() + 1, 0, (sockaddr*)&UDPHint, sizeof(UDPHint));
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



				sendto(UDPSock, q.c_str(), q.size() + 1, 0, (sockaddr*)&UDPHint, sizeof(UDPHint));


			}


			break;
		}

		}


	}



	return;

}




int main()
{


	//initialize the array to all zeros
	for (int i = 0; i < 201; i++) {
		for (int j = 0; j < 201; j++) {
			field[i][j] = 0;
		}

	}

	std::cout << "Enter port: ";
	std::cin >> serverPort;

	//initialize winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Can't start winsock, err #" << wsResult << std::endl;
		return 0;

	}



	//create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Can't create socket, err #" << WSAGetLastError << std::endl;
		WSACleanup();
		return 0;

	}



	//fill in a hint structure
	sockaddr_in hint1;
	hint1.sin_family = AF_INET;
	hint1.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIpAddress.c_str(), &hint1.sin_addr);


	//Creating a UDP socket
	std::string local = "::1";
	sockaddr_in6 hint2;
	hint2.sin6_family = AF_INET6;
	hint2.sin6_port = htons(5000);
	hint2.sin6_flowinfo = 0;
	hint2.sin6_scope_id = 0;
	inet_pton(AF_INET6, local.c_str(), &hint2.sin6_addr);
	SOCKET UDPSock = socket(AF_INET6, SOCK_DGRAM, 0);


	//connect to server
	int connResultTCP = connect(sock, (sockaddr*)&hint1, sizeof(hint1));
	if (connResultTCP == SOCKET_ERROR)
	{
		std::cerr << "Can't connect to server, Err #" << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return 0;
	}



	char buf[4096];
	std::string userInput;

	int id{};
	int seq_num{};


	std::thread receiv(receiving, sock, UDPSock, hint2);
	std::thread send(sending, sock);



	//infinite loop 
	while (true) {



	}

	return 0;
};





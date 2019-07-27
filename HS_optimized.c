#include <stdio.h>
#include <math.h>
#include "mpi.h"

#define PROBE 0
#define REPLY 1
#define LEADER 2

int main(int argc, char* argv[]) {
	int my_rank; /* rank of process */
	int p; /* number of processes */
	int tag = 0; /* tag for messages */
	MPI_Status status; /* return status for receive */
	MPI_Request req[2];
	MPI_Status stats[2];

	/* start up MPI */

	MPI_Init(&argc, &argv);

	/* find out process rank */
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	/* find out number of processes */
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	int messagesCounter = 0;
	int leaderAtPhase = -1;
	int possibleLeader = -1;
	int direction = -1;

	//int ids[] = { 7, 10, 6, 4, 2, 8 };
	int ids[] = { 90, 4, 6, 21, 8, 7, 60, 2, 3, 5, 80, 11, 15, 19, 17 };
	int left = (my_rank - 1 + p) % p;
	int right = (my_rank + 1) % p;
	int sender, leader;
	int receiveCnt = 0;

	int msg[4];
	msg[0] = PROBE;
	msg[1] = ids[my_rank];
	msg[2] = 0; // phase
	msg[3] = 1; //hops

	int maxid = ids[my_rank];

	MPI_Isend(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD, &req[0]);
	++messagesCounter;
	MPI_Isend(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD, &req[1]);
	++messagesCounter;

	while (1) {
		MPI_Waitall(2, req, stats);
		MPI_Recv(&msg, 4, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
		sender = status.MPI_SOURCE;

		if (msg[0] == PROBE) {
			//if i receive the message from the opposite direction
			//and the message has the same possibleLeader
			//send to both directions. it should stop at actual leader
			if (sender != direction && possibleLeader == msg[1]) {
				msg[0] = LEADER;
				msg[2] = -1;
				msg[3] = -1;

				printf("Process %d [%d] is announcing leader to both directions\n", my_rank, ids[my_rank]);
				MPI_Send(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD);
				++messagesCounter;
				MPI_Send(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD);
				++messagesCounter;

				leader = msg[1];
				printf("I am process %d and my leader is %d\n", ids[my_rank], leader);
				//printf("I am process %d (id %d) and i sent %d messages \n", my_rank, ids[my_rank], messagesCounter);
				break;
			}
			else
			{
				possibleLeader = msg[1];
				direction = sender;
			}

			if (msg[1] == ids[my_rank]) { //leader
				msg[0] = LEADER;
				msg[2] = -1;
				msg[3] = -1;
				MPI_Send(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD);
				++messagesCounter;
			} else if (msg[1] >= maxid && msg[3] < (1 << msg[2])) { //forward
				maxid = msg[1];
				if (leaderAtPhase != -1 && (1 << msg[2]) - msg[3] <= (1 << leaderAtPhase)) {
					//printf("process id %d leader at phase %d SAVING %d hops for id %d\n", ids[my_rank], leaderAtPhase, (1 << msg[2]) - msg[3], msg[1]);
					msg[0] = REPLY;
					msg[3] = -1;
					if (sender == left) {
						MPI_Send(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD);
						++messagesCounter;
					} else {
						MPI_Send(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD);
						++messagesCounter;
					}
				} else {
					msg[3] = msg[3] + 1;
					if (sender == right) {
						MPI_Send(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD);
						++messagesCounter;
					} else {
						MPI_Send(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD);
						++messagesCounter;
					}
				}
			} else if (msg[1] >= maxid) //hops == pow(2,phase) //border
					{ //pow(2,phase)-hops <= pow(2,leaderAtPhase)
				maxid = msg[1];
				msg[0] = REPLY;
				msg[3] = -1;
				if (sender == left) {
					MPI_Send(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD);
					++messagesCounter;
				} else {
					MPI_Send(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD);
					++messagesCounter;
				}
			}
		} else if (msg[0] == REPLY) {
			if (msg[1] >= maxid) {
				if (msg[1] != ids[my_rank]) {
					if (sender == right) {
						MPI_Send(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD);
						++messagesCounter;
					} else {
						MPI_Send(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD);
						++messagesCounter;
					}
				} else {
					if (++receiveCnt == 2) {
						receiveCnt = 0;
						++leaderAtPhase;
						//printf("###process id %d leader at phase %d \n", ids[my_rank], leaderAtPhase);
						msg[0] = PROBE;
						msg[2] = msg[2] + 1;
						msg[3] = 1;
						MPI_Isend(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD, &req[0]);
						++messagesCounter;
						MPI_Isend(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD, &req[1]);
						++messagesCounter;
					}
				}
			}
		} else {
			if (msg[0] == LEADER) {
				leader = msg[1];
				if (msg[1] != ids[my_rank]) {
					if(sender == right)
					{
						MPI_Send(&msg, 4, MPI_INT, left, tag, MPI_COMM_WORLD);
					}
					else
					{
						MPI_Send(&msg, 4, MPI_INT, right, tag, MPI_COMM_WORLD);
					}
					++messagesCounter;
				}
				printf("I am process %d and my leader is %d\n", ids[my_rank], leader);
				//printf("I am process %d (id %d) and i sent %d messages \n", my_rank, ids[my_rank], messagesCounter);
				break;
			}
		}
	}

	/* shut down MPI */
	MPI_Finalize();

	return 0;
}

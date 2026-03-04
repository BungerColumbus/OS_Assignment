/* 
 * Operating Systems (2INCO) Practical Assignment
 * Interprocess Communication
 *
 * Contains functions that are used by the clients
 *
 */

#include "request.h"

// Array of requests
//const Request requests[] = { {1, 26, 1}, {2, 5, 2}, {3, 10, 2}, {5, 13, 1}, {4, 3, 1} };	//Default case
const Request requests[] = {
    {1, 10, 1},  {2, 12, 1},  {3, 15, 1},  {4, 18, 1},  {5, 20, 1},
    {6, 22, 1},  {7, 25, 1},  {8, 28, 1},  {9, 30, 1},  {10, 32, 1},
    {11, 34, 1}, {12, 36, 1}, {13, 38, 1}, {14, 40, 1}, {15, 41, 1},
    {16, 42, 1}, {17, 43, 1}, {18, 44, 1}, {19, 45, 1}, {20, 11, 1}
};//Test case 2

// const Request requests[] = { 
//     {1, 5, 1}, {2, 10, 2}, {3, 15, 1}, {4, 20, 2}, {5, 25, 1},
//     {6, 30, 2}, {7, 35, 1}, {8, 40, 2}, {9, 45, 1}, {10, 50, 2},
//     {11, 55, 1}, {12, 60, 2}, {13, 65, 1}, {14, 70, 2}, {15, 75, 1},
//     {16, 80, 2}, {17, 85, 1}, {18, 90, 2}, {19, 95, 1}, {20, 100, 2}
// };	//Test case 3

// const Request requests[] = { 
//     // First 15 directed to Service 1
//     {1, 1, 1}, {2, 2, 1}, {3, 3, 1}, {4, 4, 1}, {5, 5, 1},
//     {6, 6, 1}, {7, 7, 1}, {8, 8, 1}, {9, 9, 1}, {10, 10, 1},
//     {11, 11, 1}, {12, 12, 1}, {13, 13, 1}, {14, 14, 1}, {15, 15, 1},
//     // Then 15 directed to Service 2
//     {16, 16, 2}, {17, 17, 2}, {18, 18, 2}, {19, 19, 2}, {20, 20, 2},
//     {21, 21, 2}, {22, 22, 2}, {23, 23, 2}, {24, 24, 2}, {25, 25, 2},
//     {26, 26, 2}, {27, 27, 2}, {28, 28, 2}, {29, 29, 2}, {30, 30, 2}
// }; // Test case 4

// Places the information of the next request in the parameters sent by reference.
// Returns NO_REQ if there is no request to make.
// Returns NO_ERR otherwise.
int getNextRequest(int* jobID, int* data, int* serviceID) {
	static int i = 0;
	static int N_REQUESTS = sizeof(requests) / sizeof(Request);

	if (i >= N_REQUESTS) 
		return NO_REQ;

	*jobID = requests[i].job;
	*data = requests[i].data;
	*serviceID = requests[i].service;		
	++i;
	return NO_ERR;
		
}

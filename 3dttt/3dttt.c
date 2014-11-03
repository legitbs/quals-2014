#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <signal.h>

#define MAX_TIME 8
#define none 0
#define X 1
#define O 2

#define VICTORY 50

int debug = 0;

typedef struct c {
	int x;
	int y;
	int z;
} cell;
struct r {
	cell P1;
	cell P2;
	cell P3;
};

struct r WR[49];

#define MAX_PREFERRED 15
cell Preferred[MAX_PREFERRED];
	
int board[3][3][3];
int PlayerToken = X;
int ComputerToken = O;

cell PossibleWins[49];
int PW_Count = 0;

int init() {
	int i = 0;
	// top row
	WR[i].P1 = (cell){0,0,0};
	WR[i].P2 = (cell){1,0,0};
	WR[i].P3 = (cell){2,0,0}; i++;

	WR[i].P1 = (cell){0,0,1};
	WR[i].P2 = (cell){1,0,1};
	WR[i].P3 = (cell){2,0,1}; i++;

	WR[i].P1 = (cell){0,0,2};
	WR[i].P2 = (cell){1,0,2};
	WR[i].P3 = (cell){2,0,2}; i++;

	// middle row
	WR[i].P1 = (cell){0,1,0};
	WR[i].P2 = (cell){1,1,0};
	WR[i].P3 = (cell){2,1,0}; i++;

	WR[i].P1 = (cell){0,1,1};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){2,1,1}; i++;

	WR[i].P1 = (cell){0,1,2};
	WR[i].P2 = (cell){1,1,2};
	WR[i].P3 = (cell){2,1,2}; i++;

	// bottom row
	WR[i].P1 = (cell){0,2,0};
	WR[i].P2 = (cell){1,2,0};
	WR[i].P3 = (cell){2,2,0}; i++;

	WR[i].P1 = (cell){0,2,1};
	WR[i].P2 = (cell){1,2,1};
	WR[i].P3 = (cell){2,2,1}; i++;

	WR[i].P1 = (cell){0,2,2};
	WR[i].P2 = (cell){1,2,2};
	WR[i].P3 = (cell){2,2,2}; i++;

	// left column
	WR[i].P1 = (cell){0,0,0};
	WR[i].P2 = (cell){0,1,0};
	WR[i].P3 = (cell){0,2,0}; i++;

	WR[i].P1 = (cell){0,0,1};
	WR[i].P2 = (cell){0,1,1};
	WR[i].P3 = (cell){0,2,1}; i++;

	WR[i].P1 = (cell){0,0,2};
	WR[i].P2 = (cell){0,1,2};
	WR[i].P3 = (cell){0,2,2}; i++;

	// middle column
	WR[i].P1 = (cell){1,0,0};
	WR[i].P2 = (cell){1,1,0};
	WR[i].P3 = (cell){1,2,0}; i++;

	WR[i].P1 = (cell){1,0,1};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){1,2,1}; i++;

	WR[i].P1 = (cell){1,0,2};
	WR[i].P2 = (cell){1,1,2};
	WR[i].P3 = (cell){1,2,2}; i++;

	// right column
	WR[i].P1 = (cell){2,0,0};
	WR[i].P2 = (cell){2,1,0};
	WR[i].P3 = (cell){2,2,0}; i++;

	WR[i].P1 = (cell){2,0,1};
	WR[i].P2 = (cell){2,1,1};
	WR[i].P3 = (cell){2,2,1}; i++;

	WR[i].P1 = (cell){2,0,2};
	WR[i].P2 = (cell){2,1,2};
	WR[i].P3 = (cell){2,2,2}; i++;

	// top left to bottom right diagonal
	WR[i].P1 = (cell){0,0,0};
	WR[i].P2 = (cell){1,1,0};
	WR[i].P3 = (cell){2,2,0}; i++;

	WR[i].P1 = (cell){0,0,1};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){2,2,1}; i++;

	WR[i].P1 = (cell){0,0,2};
	WR[i].P2 = (cell){1,1,2};
	WR[i].P3 = (cell){2,2,2}; i++;

	// top right to bottom left diagonal
	WR[i].P1 = (cell){2,0,0};
	WR[i].P2 = (cell){1,1,0};
	WR[i].P3 = (cell){0,2,0}; i++;

	WR[i].P1 = (cell){2,0,1};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){0,2,1}; i++;

	WR[i].P1 = (cell){2,0,2};
	WR[i].P2 = (cell){1,1,2};
	WR[i].P3 = (cell){0,2,2}; i++;

	//
	// cross-plane wins (rotate cube 90 degrees to left)
	//
	// top row
	WR[i].P1 = (cell){0,0,0};
	WR[i].P2 = (cell){0,0,1};
	WR[i].P3 = (cell){0,0,2}; i++;

	WR[i].P1 = (cell){1,0,0};
	WR[i].P2 = (cell){1,0,1};
	WR[i].P3 = (cell){1,0,2}; i++;

	WR[i].P1 = (cell){2,0,0};
	WR[i].P2 = (cell){2,0,1};
	WR[i].P3 = (cell){2,0,2}; i++;

	// middle row
	WR[i].P1 = (cell){0,1,0};
	WR[i].P2 = (cell){0,1,1};
	WR[i].P3 = (cell){0,1,2}; i++;

	WR[i].P1 = (cell){1,1,0};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){1,1,2}; i++;

	WR[i].P1 = (cell){2,1,0};
	WR[i].P2 = (cell){2,1,1};
	WR[i].P3 = (cell){2,1,2}; i++;

	// bottom row
	WR[i].P1 = (cell){0,2,0};
	WR[i].P2 = (cell){0,2,1};
	WR[i].P3 = (cell){0,2,2}; i++;

	WR[i].P1 = (cell){1,2,0};
	WR[i].P2 = (cell){1,2,1};
	WR[i].P3 = (cell){1,2,2}; i++;

	WR[i].P1 = (cell){2,2,0};
	WR[i].P2 = (cell){2,2,1};
	WR[i].P3 = (cell){2,2,2}; i++;

	// top left to bottom right diagonal
	WR[i].P1 = (cell){0,0,0};
	WR[i].P2 = (cell){0,1,1};
	WR[i].P3 = (cell){0,2,2}; i++;

	WR[i].P1 = (cell){1,0,0};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){1,2,2}; i++;

	WR[i].P1 = (cell){2,0,0};
	WR[i].P2 = (cell){2,1,1};
	WR[i].P3 = (cell){2,2,2}; i++;

	// top right to bottom left diagonal
	WR[i].P1 = (cell){0,0,2};
	WR[i].P2 = (cell){0,1,1};
	WR[i].P3 = (cell){0,2,0}; i++;

	WR[i].P1 = (cell){1,0,2};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){1,2,0}; i++;

	WR[i].P1 = (cell){2,0,2};
	WR[i].P2 = (cell){2,1,1};
	WR[i].P3 = (cell){2,2,0}; i++;

	//
	// diagonal cross-plan wins
	//
	// front top left to bottom back right diagonal
	WR[i].P1 = (cell){0,0,0};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){2,2,2}; i++;

	// front top right to bottom back left diagonal
	WR[i].P1 = (cell){2,0,0};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){0,2,2}; i++;

	// back top left to front bottom right diagnoal
	WR[i].P1 = (cell){0,0,2};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){2,2,0}; i++;

	// back top right to front bottom left diagnoal
	WR[i].P1 = (cell){2,0,2};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){0,2,0}; i++;

	//
	// top surface wins
	//
	// front left to back right
	WR[i].P1 = (cell){0,0,0};
	WR[i].P2 = (cell){1,0,1};
	WR[i].P3 = (cell){2,0,2}; i++;

	// front right to back left
	WR[i].P1 = (cell){2,0,0};
	WR[i].P2 = (cell){1,0,1};
	WR[i].P3 = (cell){0,0,2}; i++;

	//
	// middle surface wins
	//
	// front left to back right
	WR[i].P1 = (cell){0,1,0};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){2,1,2}; i++;

	// front right to back left
	WR[i].P1 = (cell){2,1,0};
	WR[i].P2 = (cell){1,1,1};
	WR[i].P3 = (cell){0,1,2}; i++;

	//
	// bottom surface wins
	//
	// front left to back right
	WR[i].P1 = (cell){0,2,0};
	WR[i].P2 = (cell){1,2,1};
	WR[i].P3 = (cell){2,2,2}; i++;

	// front right to back left
	WR[i].P1 = (cell){2,2,0};
	WR[i].P2 = (cell){1,2,1};
	WR[i].P3 = (cell){0,2,2}; i++;

}

void init_preferred(void) {
	cell p[MAX_PREFERRED];
	int i;
	int r;

	p[0] = (cell){1,1,1}; // middle
	p[1] = (cell){0,0,0}; // top front left
	p[2] = (cell){0,0,2}; // top back left
	p[3] = (cell){2,0,0}; // top front right
	p[4] = (cell){2,0,2}; // top back right
	p[5] = (cell){0,2,0}; // bottom front left
	p[6] = (cell){0,2,2}; // bottom back left
	p[7] = (cell){2,2,0}; // bottom front right
	p[8] = (cell){2,2,2}; // bottom back right
	p[9] = (cell){1,0,1}; // middle of top face
	p[10] = (cell){0,1,1}; // middle of left face
	p[11] = (cell){2,1,1}; // middle of right face
	p[12] = (cell){1,2,1}; // middle of bottom face
	p[13] = (cell){1,1,0}; // middle of front face
	p[14] = (cell){1,1,2}; // middle of back face

	// init the Preferred array to unallocated values
	for (i = 0; i < MAX_PREFERRED; i++) {
		Preferred[i] = (cell){-1,-1,-1};
	}

	// randomly init the list of preferred cells 
	for (i = 0; i < MAX_PREFERRED; i++) {
		r = rand() % MAX_PREFERRED;	
		while (Preferred[r].x != -1) {
			r = rand() % MAX_PREFERRED;	
		}
		Preferred[r] = p[i];
	}

//	for (i = 0; i < MAX_PREFERRED; i++) {
//		printf("%d,%d,%d\n", Preferred[i].x,Preferred[i].y,Preferred[i].z);
//	}

}

void MakeNextMove() {
	int board_test[3][3][3];
	int w1;
	int r;
	int x,y,z;
	int NumWinningRows;
	int BestSolutions[49];
	int CountSolutions;
	int Best;
	int MaxWinningRows;
	int i,j;
	int Curr_X_Wins;
	int New_X_Wins;
	int Max_X_Wins;
	cell Max_X_Cell;
	int Max_O_Wins;
	cell Max_O_Cell;
	int win_count;

	// calc current number of winning rows for the Player
	Curr_X_Wins = CalcWinningRows(board, PlayerToken, 0);

	// for all open spaces on the board
	Max_X_Wins = -1;
	for (x = 0; x < 3; x++) {
		for (y = 0; y < 3; y++) {
			for (z = 0; z < 3; z++) {
				if (board[x][y][z] == none) {
					// calc winnning rows if X were to move there next
					bcopy(board, board_test, sizeof(board));
					board_test[x][y][z] = PlayerToken;
//					printf("testing: %d,%d,%d\n", x,y,z);
					New_X_Wins = CalcWinningRows(board_test, PlayerToken, 1) - Curr_X_Wins;
					if (New_X_Wins > Max_X_Wins) {
						Max_X_Wins = New_X_Wins;
						Max_X_Cell.x = x;
						Max_X_Cell.y = y;
						Max_X_Cell.z = z;
					}
				}
			}
		}
	}

	// if there's a chance for X to win more than one row with the next move, block it
	if (Max_X_Wins > 1) {
		board[Max_X_Cell.x][Max_X_Cell.y][Max_X_Cell.z] = ComputerToken;
		if (debug) {
			printf("X could win %d times by choosing %d,%d,%d, so we're blocking it\n",
				Max_X_Wins, Max_X_Cell.x, Max_X_Cell.y, Max_X_Cell.z);
		}
		return;
	}
		
	// find a next move for us that would lead to us scoring
	FindPossibleWins(ComputerToken);

	// is there any next move which would result in us scoring more than once?
	Max_O_Wins = -1;
	for (i = 0; i < PW_Count; i++) {
		win_count = 0;
		for (j = i; j < PW_Count; j++) {
			if (PossibleWins[i].x == PossibleWins[j].x && PossibleWins[i].y == PossibleWins[j].y && PossibleWins[i].z == PossibleWins[j].z) {
				win_count++;
				if (win_count > Max_O_Wins) {
					Max_O_Wins = win_count;
					Max_O_Cell = PossibleWins[j];
				}
			}
		}
	}
	if (Max_O_Wins > 1) {
		// we have a choice which would lead to us scorimg more than once, use that
		board[Max_O_Cell.x][Max_O_Cell.y][Max_O_Cell.z] = ComputerToken;
		if (debug) {
			printf("We have a cell (%d,%d,%d) which would result in %d wins, so we choose that one\n",
				Max_O_Cell.x, Max_O_Cell.y, Max_O_Cell.z, Max_O_Wins);
		}
		return;
	}
		
	// no win would let us score more than once, so pick a possible win and use that
	if (PW_Count == 1) {
		board[PossibleWins[0].x][PossibleWins[0].y][PossibleWins[0].z] = ComputerToken;
		if (debug) 
			printf("we only have one win\n");
		return;
	} else if (PW_Count >= 2) {
		// randomize if we have multiple choices
		r = rand() % PW_Count;
		board[PossibleWins[r].x][PossibleWins[r].y][PossibleWins[r].z] = ComputerToken;
		if (debug) 
			printf("we have %d possible wins, choosing %d\n", PW_Count, r);
		return;
	} 

	if (debug)
		printf("we have no pending wins, so see if the player is about to win\n");

	// see how many times the player is about to score
	FindPossibleWins(PlayerToken);
	if (debug)  {
		for (i = 0; i < PW_Count; i++) {
				printf("%d,%d,%d\n", PossibleWins[i].x,PossibleWins[i].y,PossibleWins[i].z);
		}
	}
	if (PW_Count == 1) {
		// if one, block him
		board[PossibleWins[0].x][PossibleWins[0].y][PossibleWins[0].z] = ComputerToken;
		if (debug)
			printf("block player's single possible win\n");
		return;
	} else if (PW_Count >= 2) {
		// if two or more, see which of first two blocking moves would be in our best interest
		if (debug)
			printf("player has %d possible wins\n", PW_Count);

		// calc the # Winning Rows for each possible solution
		CountSolutions = 0;
		MaxWinningRows = -1;
		for (i = 0; i < PW_Count; i++) {
			// make a copy of the current board
			bcopy(board, board_test, sizeof(board));

			// set our token in the current possible location
			board_test[PossibleWins[i].x][PossibleWins[i].y][PossibleWins[i].z] = ComputerToken;

			// calculate the overall number of winning rows given that move
			NumWinningRows = CalcWinningRows(board_test, ComputerToken, 0);

			// see if the new winning rows is better than we've found before
			if (NumWinningRows > MaxWinningRows) {
				MaxWinningRows = NumWinningRows;
				CountSolutions = 0;
				BestSolutions[CountSolutions++] = i;
			} else if (NumWinningRows == MaxWinningRows) {
				BestSolutions[CountSolutions++] = i;
			}
		}

		// choose the best solution randomly from the equally best choices
		Best = BestSolutions[rand() % CountSolutions];
		if (debug) {
			for (i = 0; i < CountSolutions; i++) {
				printf("  %d,%d,%d\n", 
					PossibleWins[BestSolutions[i]].x,PossibleWins[BestSolutions[i]].y,PossibleWins[BestSolutions[i]].z);
			}
		}
		board[PossibleWins[Best].x][PossibleWins[Best].y][PossibleWins[Best].z] = ComputerToken;
		if (debug) {
			printf("Choosing %d,%d,%d randomly from %d equal solutions for the block\n", 
				PossibleWins[Best].x,PossibleWins[Best].y,PossibleWins[Best].z, CountSolutions);
		}

		return;
	}
	
	if (debug) {
		printf("player has no pending wins\n");
		printf("we have no possible wins, try to choose one of the preferred cells\n");
	}
	// always choose the center of the cube first if it's available
	if (board[Preferred[0].x][Preferred[0].y][Preferred[0].z] == none) {
		board[Preferred[0].x][Preferred[0].y][Preferred[0].z] = ComputerToken;
		return;
	}
	// otherwise, choose one of the remaining cells randomly
	for (i = 1; i < MAX_PREFERRED; i++) {
		if (board[Preferred[i].x][Preferred[i].y][Preferred[i].z] == none) {
			board[Preferred[i].x][Preferred[i].y][Preferred[i].z] = ComputerToken;
			return;
		}
	}
	if (debug)
		printf("all preferred cells are taken, choose randomly\n");
	// no good choices, pick randomly
	x = rand()%3; y = rand()%3; z = rand()%3;
	while (board[x][y][z] != none) {
		x = rand()%3; y = rand()%3; z = rand()%3;
	}
	board[x][y][z] = ComputerToken;
	return;
}

void PrintBoard() {
	int x,y,z;

	for (z = 0; z < 3; z++) {
		printf(" x 0   1   2    z=%d\ny\n",z);
		for (y = 0; y < 3; y++) {
			printf("%d ", y);
			for (x = 0; x < 3; x++) {
				if (board[x][y][z] == none) {
					printf("   ");
				} else if (board [x][y][z] == X) {
					printf(" X ");
				} else if (board [x][y][z] == O) {
					printf(" O ");
				}
				if (x != 2) {
					printf("|");
				}
			}
			printf("\n");
			if (y != 2) {
				printf("  ---+---+---\n");
			}
		}
		//printf("  -------------\n");
		printf("\n\n");
	}
}

void RandomInit() {
	int x,y,z;

	for (z = 0; z < 3; z++) {
		for (y = 0; y < 3; y++) {
			for (x = 0; x < 3; x++) {
				board[x][y][z] = (rand() % 2)+1;
			}
		}
	}
}

int CheckRowForPossibleWin(int Q, int A, int B, int C) {
	if ( (A == Q && B == Q && C == none) ||
	     (A == Q && B == none && C == Q) ||
	     (A == none && B == Q && C == Q)) {
		return(1);
	} else {
		return(0);
	}
}

int CheckForPossibleWin(int b[3][3][3], int Q, int i) {
	if ( b[WR[i].P1.x][WR[i].P1.y][WR[i].P1.z] == Q && 
	     b[WR[i].P2.x][WR[i].P2.y][WR[i].P2.z] == Q && 
	     b[WR[i].P3.x][WR[i].P3.y][WR[i].P3.z] == none) {
		PossibleWins[PW_Count++] = WR[i].P3;
		return(1);
	} 

	if ( b[WR[i].P1.x][WR[i].P1.y][WR[i].P1.z] == Q && 
	     b[WR[i].P2.x][WR[i].P2.y][WR[i].P2.z] == none && 
	     b[WR[i].P3.x][WR[i].P3.y][WR[i].P3.z] == Q) {
		PossibleWins[PW_Count++] = WR[i].P2;
		return(1);
	} 

	if ( b[WR[i].P1.x][WR[i].P1.y][WR[i].P1.z] == none && 
	     b[WR[i].P2.x][WR[i].P2.y][WR[i].P2.z] == Q && 
	     b[WR[i].P3.x][WR[i].P3.y][WR[i].P3.z] == Q) {
		PossibleWins[PW_Count++] = WR[i].P1;
		return(1);
	} 

	return(0);
}

int FindPossibleWins(int Q) {
	int i;
	int Winning_Rows;

	// check each possible row and see if putting Q into the remaining
	// space would win that row

	// store the positions which would result in a winning row
	// into the array PossibleWins

	// init PossibleWins
	bzero(PossibleWins, sizeof(PossibleWins));
	PW_Count = 0;

	for (i = 0; i < 49; i++) {
		CheckForPossibleWin(board, Q, i);
	}
}

int PrintWinningRow(int i) {
	printf("(%d,%d,%d) (%d,%d,%d) (%d,%d,%d)\n", 
		WR[i].P1.x, WR[i].P1.y, WR[i].P1.z,
		WR[i].P2.x, WR[i].P2.y, WR[i].P2.z,
		WR[i].P3.x, WR[i].P3.y, WR[i].P3.z);
}

int CheckForWin(int b[3][3][3], int Q, int i) {
	if ( b[WR[i].P1.x][WR[i].P1.y][WR[i].P1.z] == Q && 
	     b[WR[i].P2.x][WR[i].P2.y][WR[i].P2.z] == Q && 
	     b[WR[i].P3.x][WR[i].P3.y][WR[i].P3.z] == Q) {
		return(1);
	} else {
		return(0);
	}
}

int CalcWinningRows(int b[3][3][3], int Q, int chatty) {
	int i;
	int Winning_Rows = 0;

	for (i = 0; i < 49; i++) {
		if (CheckForWin(b, Q, i)) {
//			if (chatty) {
//				PrintWinningRow(i);
//			}
			Winning_Rows++;
		}
	}

	return(Winning_Rows);
}

void PlayerInput() {
	char line[100];
	int x,y,z;
	int done = 0;

	alarm(MAX_TIME);
	while (!done) {
		printf("Choose Wisely (x,y,z): ");
		fflush(stdout);
		fgets(line, 99, stdin);
		if (strlen(line) > 6) {
			continue;
		}
		if (line[0] != '0' && line[0] != '1' && line[0] != '2') {
			continue;
		} else {
			x = line[0]-48;
		}
		if (line[1] != ',') {
			continue;
		}
		if (line[2] != '0' && line[2] != '1' && line[2] != '2') {
			continue;
		} else {
			y = line[2]-48;
		}
		if (line[3] != ',') {
			continue;
		}
		if (line[4] != '0' && line[4] != '1' && line[4] != '2') {
			continue;
		} else {
			z = line[4]-48;
		}
//		if (x < 0 || x > 2) {
//			continue;
//		}
//		if (y < 0 || y > 2) {
//			continue;
//		}
//		if (z < 0 || z > 2) {
//			continue;
//		}
		if (board[x][y][z] != none) {
			continue;
		}
		board[x][y][z] = PlayerToken;
		alarm(0);
		done = 1;
	}

}

int IsBoardFull() {
	int x,y,z;

	for (x = 0; x < 3; x++) {
		for (y = 0; y < 3; y++) {
			for (z = 0; z < 3; z++) {
				if (board[x][y][z] == none) {
					return(0);
				}
			}
		}
	}
	return(1);
}

void Welcome(void) {

	printf("LBS (#) welcomes you to a friendly game of 3D Tic Tac Toe\n\n");
	printf("Since drawing 3D ASCII art is hard, you'll have to use your\n");
	printf("imagination to picture the game board below in 3D.  The z=0\n");
	printf("board is the side of the cube closest to you.  z=1 is in the middle\n");
	printf("and z=2 is the side farthest from you.\n\n");
	printf("The player with the MOST winning rows wins the round.\n\n");
	printf("Play well and play fast....\n\n");

}
	
static void TooSlow(int signo) {
	printf("\nMust go faster...\n");
	exit(0);
}

int main(void) {
	int X_Winning_Rows;
	int O_Winning_Rows;
	int rounds = 0;
	int RoundsWon = 0;
	FILE *key;
	char key_buf[1000];

	signal(SIGALRM, TooSlow);

	srand(time(NULL));

	init();
	//RandomInit();

	Welcome();

	while (RoundsWon < VICTORY && RoundsWon > -5) {
		init_preferred();
		bzero(board, sizeof(board));
		while (1) {
			PrintBoard();
			X_Winning_Rows = CalcWinningRows(board, X, 0);
			printf("X Winning Rows: %d\n", X_Winning_Rows);
			O_Winning_Rows = CalcWinningRows(board, O, 0);
			printf("O Winning Rows: %d\n", O_Winning_Rows);
			PlayerInput();
			if (IsBoardFull()) {
				break;
			}
			MakeNextMove();
		}
		PrintBoard();
		X_Winning_Rows = CalcWinningRows(board, X, 1);
		printf("X Winning Rows: %d\n", X_Winning_Rows);
		O_Winning_Rows = CalcWinningRows(board, O, 1);
		printf("O Winning Rows: %d\n", O_Winning_Rows);
		if (X_Winning_Rows > O_Winning_Rows) {
			RoundsWon++;
		} else if (X_Winning_Rows < O_Winning_Rows) {
			RoundsWon--;
		}
		if (RoundsWon > -5) {
			printf("You've won %d rounds...Let's play again\n\n", RoundsWon);
		} else {
			printf("You've won %d rounds...\n\n", RoundsWon);
		}
	}

	if (RoundsWon == VICTORY) {
		printf("Just kidding...congrats!\n");
		if ((key = fopen("/home/3dttt/flag", "r")) == NULL) {
			printf("Oh shit...the flag file is missing...you should probably let someone know\n");
			fflush(stdout);
			exit(0);
		}
		fgets(key_buf, 999, key);
		fclose(key);
		printf("%s\n", key_buf);
	} else {
		printf("Play better!\n");
	}

	return(0);
}



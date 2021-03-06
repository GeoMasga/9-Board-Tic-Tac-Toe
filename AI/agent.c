/*********************************************************
 *  Agent.c
 *  Nine-Board Tic-Tac-Toe Agent
 *  COMP3411/9414/9814 Artificial Intelligence
 *  Rudi Purnomo z3410682
 *  Group 106
 *  based on Alan Blair's agent.c provided in the source code
 */

 /*********************************************************

 Note:(I will use this term in explaining and commenting)
 Board = Whole game board, all 9 sub-boards
 Sub-Board = Consist of 3x3 tile
 Tile = Single tile, can be owned by player, opponent(!player), or empty
 Adjacency = Combination that makes legit 3 straight winning tiles in tic-tac-toe

 Code explanation
 My code works using the standard alpha beta pruning as the base idea.
 In order to keep in track of score for every move during pruning, 
 a board is given score based on the tile ownership distribution for every adjacency in all sub-boards.
 The constant value is choosen in order to put 1 adjacency completion in much higher priority rather than distributing tile evenly in all adjacency.
 Then for every move, the code will try all legit move(numbering order), and find their alpha beta score from it, 
 where the highest scoring move will be choosen as that turn move.
 If there is a situation where 2 or more moves have the same score, 
 one will be choosen randomly.

 Existing problem:
 Method to find the children in alpha beta pruning in best order,
 current method is just randomize the order of the legit children, prioritizing edge for early game moves
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "common.h"
#include "agent.h"
#include "game.h"

#define MAX_MOVE 81
// Adjacency value
#define EMPTY_ADJACENCY 1 //only empty tiles
#define DEAD_ADJACENCY 0 //Consist of tile owned by both player and opponent, thus can't be completed
#define PLAYER_1_ADJACENCY 100 //consist of 1 player tile and 2 empty tiles
#define PLAYER_2_ADJACENCY 10000 //consist of 2 player tiles and 1 empty tile
#define PLAYER_3_ADJACENCY 1000000 //consist of 3 player tiles (WIN)
#define OPPONENT_1_ADJACENCY -100 //consist of 1 opponent tile and 2 empty tiles
#define OPPONENT_2_ADJACENCY -10000 //consist of 2 opponent tiles and 1 empty tile
#define OPPONENT_3_ADJACENCY -1000000 //consist of 3 opponent tiles (LOSE)
//Alpha beta constant
#define DEPTH 8 //Most reasonable time
#define ALPHA_START -1000000 //Start with opposing win value
#define BETA_START 1000000 //Start with player win value
#define WIN_LIMIT 900000 //This move is a win (taking into consideration the possibility of opposing negative value)
#define LOSE_LIMIT -900000 //This move is a lose (taking into consideration the possibility of player negative value)
//Extra
#define EARLY_GAME 10 //Turn that is considered early game

int board[10][10];
int move[MAX_MOVE+1];
int player;
int m; //Turns passed

/*********************************************************//*
  Adjacency lists and their index number
*/
int adj[8][3] = {
  //Row adjacent
  {1,2,3}, //0
  {4,5,6}, //1
  {7,8,9}, //2
  //Column adjacent 
  {1,4,7}, //3
  {2,5,8}, //4
  {3,6,9}, //5
  //Diagonal adjacent
  {1,5,9}, //6
  {3,5,7}  //7
};

/*********************************************************//*
  Tile priority in early game
*/
int prio[9] = {1,3,7,9,5,2,4,6,8}; 

/*********************************************************//*
   Print usage information and exit
*/
void usage( char argv0[] )
{
  printf("Usage: %s\n",argv0);
  printf("       [-p port]\n"); // tcp port
  printf("       [-h host]\n"); // tcp host
  exit(1);
}

/*********************************************************//*
   Parse command-line arguments
*/
void agent_parse_args( int argc, char *argv[] )
{
  int i=1;
  while( i < argc ) {
    if( strcmp( argv[i], "-p" ) == 0 ) {
      if( i+1 >= argc ) {
        usage( argv[0] );
      }
      port = atoi(argv[i+1]);
      i += 2;
    }
    else if( strcmp( argv[i], "-h" ) == 0 ) {
      if( i+1 >= argc ) {
        usage( argv[0] );
      }
      host = argv[i+1];
      i += 2;
    }
    else {
      usage( argv[0] );
    }
  }
}

/*********************************************************//*
   Called at the beginning of a series of games
*/
void agent_init()
{
  struct timeval tp;
  // generate a new random seed each time
  gettimeofday( &tp, NULL );
  srandom(( unsigned int )( tp.tv_usec ));
}

/*********************************************************//*
   Called at the beginning of each game
*/
void agent_start( int this_player )
{
  reset_board( board );
  m = 0;
  move[m] = 0;
  player = this_player;
}

/*********************************************************//*
   Choose second move and return it
*/
int agent_second_move( int board_num, int prev_move )
{
  int this_move = 0;
  int this_move_value = 0;
  move[0] = board_num;
  move[1] = prev_move;
  board[board_num][prev_move] = !player;
  m = 2;
  int x = 1;
  while (x < 10) {
    if(board[prev_move][x] == EMPTY) {
      int board_alpha[10][10];
      int ab_value;
      int copy_x, copy_y;
      for(copy_x = 1 ; copy_x < 10 ; copy_x++) {
        for(copy_y = 1 ; copy_y < 10 ; copy_y++) {
          board_alpha[copy_x][copy_y] = board[copy_x][copy_y];
        }
      }
      board_alpha[prev_move][x] = player;
      ab_value = move_check_ab(!player, board_alpha, DEPTH, ALPHA_START, BETA_START, x);
      if(this_move != 0) {
        if(ab_value > this_move_value) {
          this_move = x;
          this_move_value = ab_value;
        } else if(ab_value == this_move_value) {
          int dummy = rand();
          if(dummy % 2 == 1) {
            this_move = x;
          }
        }
      } else {
        this_move = x;
        this_move_value = ab_value;
      }
    }
    x++;
  }
  x = 0;
  move[m] = this_move;
  board[prev_move][this_move] = player;
  return( this_move );
}

/*********************************************************//*
   Choose third move and return it
*/
int agent_third_move(
                     int board_num,
                     int first_move,
                     int prev_move
                    )
{
  int this_move = 0;
  int this_move_value = 0;
  move[0] = board_num;
  move[1] = first_move;
  move[2] = prev_move;
  board[board_num][first_move] =  player;
  board[first_move][prev_move] = !player;
  m=3;
  int x = 1;
  while (x < 10) {
    if(board[prev_move][x] == EMPTY) {
      int board_alpha[10][10];
      int ab_value;
      int copy_x, copy_y;
      for(copy_x = 1 ; copy_x < 10 ; copy_x++) {
        for(copy_y = 1 ; copy_y < 10 ; copy_y++) {
          board_alpha[copy_x][copy_y] = board[copy_x][copy_y];
        }
      }
      board_alpha[prev_move][x] = player;
      ab_value = move_check_ab(!player, board_alpha, DEPTH, ALPHA_START, BETA_START, x);
      if(this_move != 0) {
        if(ab_value > this_move_value) {
          this_move = x;
          this_move_value = ab_value;
        } else if(ab_value == this_move_value) {
          int dummy = rand();
          if(dummy % 2 == 1) {
            this_move = x;
          }
        }
      } else {
        this_move = x;
        this_move_value = ab_value;
      }
    }
    x++;
  }
  move[m] = this_move;
  board[move[m-1]][this_move] = player;
  return( this_move );
}

/*********************************************************//*
   Choose next move and return it
*/
int agent_next_move( int prev_move )
{
  int this_move = 0;
  int this_move_value = 0;
  m++;
  move[m] = prev_move;
  board[move[m-1]][move[m]] = !player;
  m++;
  int x = 1;
  while (x < 10) {
    if(board[prev_move][x] == EMPTY) {
      int board_alpha[10][10];
      int ab_value;
      int copy_x, copy_y;
      for(copy_x = 1 ; copy_x < 10 ; copy_x++) {
        for(copy_y = 1 ; copy_y < 10 ; copy_y++) {
          board_alpha[copy_x][copy_y] = board[copy_x][copy_y];
        }
      }
      board_alpha[prev_move][x] = player;
      ab_value = move_check_ab(!player, board_alpha, DEPTH, ALPHA_START, BETA_START, x);
      if(this_move != 0) {
        if(ab_value > this_move_value) {
          this_move = x;
          this_move_value = ab_value;
        } else if(ab_value == this_move_value) {
          int dummy = rand();
          if(dummy % 2 == 1) {
            this_move = x;
          }
        }
      } else {
        this_move = x;
        this_move_value = ab_value;
      }
    }
    x++;
  }
  move[m] = this_move;
  board[move[m-1]][this_move] = player;
  return( this_move );
}

/*********************************************************//*
   Receive last move and mark it on the board
*/
void agent_last_move( int prev_move )
{
  m++;
  move[m] = prev_move;
  board[move[m-1]][move[m]] = !player;
}

/*********************************************************//*
   Called after each game
*/
void agent_gameover(
                    int result,// WIN, LOSS or DRAW
                    int cause  // TRIPLE, ILLEGAL_MOVE, TIMEOUT or FULL_BOARD
                   )
{
  // nothing to do here
}

/*********************************************************//*
   Called after the series of games
*/
void agent_cleanup()
{
  // nothing to do here
}


/*********************************************************//*
    Original function
*/

/*********************************************************//*
    Adjacency value calculation
    Return the value based on the number of tiles filled
*/
int adj_value(int count_player, int count_opponent) {
  int value;
  if (count_player > 0 && count_opponent == 0) {  //for adjacency with only our tile and empty tile give it score 10^number of owned tiles
    if (count_player == 1) {
      value = PLAYER_1_ADJACENCY;
    } else if (count_player == 2) {
      value = PLAYER_2_ADJACENCY;
    } else {
      value = PLAYER_3_ADJACENCY;
    }
  } else if (count_player == 0 && count_opponent > 0) { //for adjacency with only opponent tile and empty tile give it score -(10^number of opposing tiles)
    if (count_opponent == 1) {
      value = OPPONENT_1_ADJACENCY;
    } else if (count_opponent == 2) {
      value = OPPONENT_2_ADJACENCY;
    } else {
      value = OPPONENT_3_ADJACENCY;
    }
  } else if (count_player > 0 && count_opponent > 0) { //adjacency which has both player own tile and opponent tile, which is a dead adjacency, give it score 0
    value = DEAD_ADJACENCY;
  } else { //both player and the opponent haven't marked any tile of these adjacency, give it score 1
    value = EMPTY_ADJACENCY;
  }
  return value;
}

/*********************************************************//*
    Total board value
    Returning the total board value using adjacency value calculation for the board
*/
int total_board_all(int board_alpha[10][10]) {
  int sum = 0;
  int count_player, count_opponent;
  int x, y, z;
   for(x = 1 ; x < 10 ; x++) {
    for(y = 0 ; y < 8 ; y++) {
      count_player = 0;
      count_opponent = 0;
      for(z = 0 ; z < 3 ; z++) {
       if (board_alpha[x][adj[y][z]] == player) {
         count_player++;
       } else if (board_alpha[x][adj[y][z]] == !player) {
         count_opponent++;
       }
      }
      sum += adj_value(count_player, count_opponent);
    }
  }
  return sum;
}


/*********************************************************//*
    Alpha beta pruning
*/
int move_check_ab(int current, int board_alpha[10][10], int depth, int alpha, int beta, int last_move) {
  int score;
  int current_score = total_board_all(board_alpha);
  if(current_score >= WIN_LIMIT || current_score <= LOSE_LIMIT || depth == 0) {
    return current_score;
  }
  int x;
  int child[9];
  int child_index = 0;
  if (m + (DEPTH - depth) <= EARLY_GAME) {
    for(x = 0 ; x < 9 ; x++) {
      if(board_alpha[last_move][prio[x]] == EMPTY) {
        child[child_index] = prio[x];
        child_index++;
      }
    }
    shuffle(child, (child_index/2) + 1);
  } else {
    for(x = 1 ; x < 10 ; x++) {
      if(board_alpha[last_move][x] == EMPTY) {
        child[child_index] = x;
        child_index++;
      }
    }
    shuffle(child, child_index);
  }
  if(current == player) {
    for(x = 0; x < child_index; x++) {
      int board_alpha_copy[10][10];
      int copy_x, copy_y;
      for(copy_x = 1 ; copy_x < 10 ; copy_x++) {
        for(copy_y = 1 ; copy_y < 10 ; copy_y++) {
          board_alpha_copy[copy_x][copy_y] = board_alpha[copy_x][copy_y];
        }
      }
      board_alpha_copy[last_move][child[x]] = player;
      score = move_check_ab(!current, board_alpha_copy, depth-1, alpha, beta, child[x]);
      if(score > alpha) {
        alpha = score;
      }
      if(alpha >= beta) {
        break;
      }
    }
    return alpha;
  } else {
    for(x = 0; x < child_index; x++) {
      int board_alpha_copy[10][10];
      int copy_x, copy_y;
      for(copy_x = 1 ; copy_x < 10 ; copy_x++) {
        for(copy_y = 1 ; copy_y < 10 ; copy_y++) {
          board_alpha_copy[copy_x][copy_y] = board_alpha[copy_x][copy_y];
        }
      }
      board_alpha_copy[last_move][child[x]] = !player;
      score = move_check_ab(!current, board_alpha_copy, depth-1, alpha, beta, child[x]);
      if(score < beta) {
        beta = score;
      }
      if(alpha >= beta) {
        break;
      }
    }
    return beta;
  }
}


void shuffle(int *array, size_t n) {
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

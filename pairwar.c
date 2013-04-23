#include<stdlib.h>
#include<pthread.h>
#include<stdio.h>


struct Deque{
	int val;
	struct Deque* next;
};

pthread_mutex_t mutex;
pthread_cond_t condArray[4];
pthread_cond_t startCond[4];
pthread_cond_t start_game;
int startArray[4] = {0,0,0,0};
struct Deque* head = NULL;
struct Deque* tail = NULL;
int winner = 0;
int startup = 0;
int player_hand[4];
int draw;
int starting_player = 1;
int current_round = 0;
int current_player = 0;
int dealer_ready = 0;

void push_back(int val){
	
	struct Deque* newNode = (struct Deque*)malloc(sizeof(struct Deque));
	newNode->val = val;
	newNode->next = NULL;
	
	if(tail != NULL){
		tail->next = newNode;
		tail = tail->next;
	}else{
		tail = newNode;
		head = newNode;
	}
	
}


int pop(){
	
	int val;
	struct Deque*  nodePtr = head;
	if(head != NULL)
		val = head->val;
	head = head->next;
	free(nodePtr);
	return val;
	
}

int dequeSize(){
	
	int size = 0;
	struct Deque* nodePtr;
	nodePtr = head;
	
	while(nodePtr != NULL){
		nodePtr=nodePtr->next;
		size++;
	}
	return size;

}

void displayDeque(){
	
	struct Deque* nodePtr;
	nodePtr = head;

	printf("Displaying Deque\n");	
	while(nodePtr != NULL){
		printf("%d ", nodePtr->val);
		nodePtr = nodePtr->next;
	}
	printf("\n");
}



void generateDeque(){
	int i, j;
	for(i=0; i<13; i++)
		for(j=0; j<4; j++)
			push_back(i+1);
}



void destroyDeque(){

	
	struct Deque* nodePtr = head;
	struct Deque* prevNode = head;

	while(nodePtr != NULL){
		prevNode = nodePtr;
		nodePtr = nodePtr->next;
		free(prevNode);
	}

	head = NULL;
	tail = NULL;
}


void shuffleDeque(void* val){
	
	struct Deque* nodePtr;
	struct Deque* shuffleNode;
	struct Deque* prevNode;
	int i, j, s, r;
	long seed = (long)val;

	//start nodePtr at head
	nodePtr = head;
	//dequesize - 2 along with the inner for loop ensure that
	//we will not go out of bounds with our randomizing;
	//praise Jesus, this took me forever to figure out
	s = dequeSize() - 2;
	srand(seed);
	
	//iterate enought times to touch at least each element of the deque and place
	//it in a random spot
	for(i = 0; i < 52; i++){

		shuffleNode = malloc(sizeof(struct Deque));
		shuffleNode->val = head->val;
		shuffleNode->next = NULL;
		nodePtr = head->next;
		free(head);
		head = nodePtr;
		r = (rand() % s);

		for(j = 0; j <= r % s; j++){
			prevNode = nodePtr;
			nodePtr = nodePtr->next;
		}
		
		if(nodePtr == NULL){
			prevNode->next = shuffleNode;
		}else{
			shuffleNode->next = nodePtr;
			prevNode->next = shuffleNode;
		}
	}
	
	//restore tail pointer
	nodePtr = head;
	while(nodePtr->next != NULL)
		nodePtr = nodePtr->next;
	tail = nodePtr;
}

int signalNext(){
	//using varialbes 
	//current_round = 0
	//current_player = 0
	//starting_player = 1
	
	//ROUND 1
	if(current_round == 1){
		
		if(current_player == 3)
			current_player = -1;
		return ++current_player;
		
	}else if(current_round == 2){
		
		if(current_player == 0){
			return 2;
		}else if(current_player == 1){
			current_player = -1;
			return ++current_player;
		}else if(current_player == 3)
			return 1;
		
	}else if(current_round == 3){
		
		if(current_player == 0)
			return 3;
		else if(current_player == 2){
			current_player = -1;
			return ++current_player;
		}else if(current_player == 3){
			current_player = 0;
			return ++current_player;
		}
		
	}
	
	if(current_player == 3 && current_round == 1)
		current_player = -1;
	return ++current_player;
}

void* dealer(void* seed){

	pthread_mutex_lock(&mutex);

	startArray[0] = 1;

	if(startArray[1] == 0){
		printf("Dealer waiting for thread %d\n", 1);
		pthread_cond_wait(&startCond[1], &mutex);
	}
	if(startArray[2] == 0){
		printf("Dealer waiting for thread %d\n", 2);
		pthread_cond_wait(&startCond[2], &mutex);
	}
	if(startArray[3] == 0){
		printf("Dealer waiting for thread %d\n", 3);
		pthread_cond_wait(&startCond[3], &mutex);
	}
	
	printf("Dealer broadcasting\n");
	pthread_cond_broadcast(&startCond[0]);

	printf("Dealer waiting for game start\n"); fflush(stdout);
	pthread_cond_wait(&start_game, &mutex);

	printf("Dealer starting\n");
	pthread_mutex_unlock(&mutex);

	while(current_round < 3){

		pthread_mutex_lock(&mutex);

		if( current_round != 0 ){
			printf("dealer waiting for signal\n");
			dealer_ready = 1;
			pthread_cond_signal(&condArray[0]);
			pthread_cond_wait(&condArray[0], &mutex);
		}

		if( current_round == 0 )
			current_round++;

		printf("\n\n  ROUND %d\n", current_round);

		//shuffle deque
		displayDeque();
		printf("Dealer Shuffling\n"); fflush(stdout);
		shuffleDeque(seed);
		displayDeque();

		//Dealer deals
		printf("Dealer dealing:\n");
		player_hand[1] = pop();
		player_hand[2] = pop();
		player_hand[3] = pop();

		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&condArray[1]);
	}

	printf("DEALER EXITING\n");
	pthread_exit(NULL);
}


void* player1(void* val){


	long my_rank = (long)val;
	long signal_next = my_rank + 1;

	pthread_mutex_lock(&mutex);

	startArray[my_rank] = 1;

	if(startArray[0] == 0){
		printf("Thread %d waiting for dealer\n", my_rank);
		pthread_cond_wait(&startCond[0], &mutex);
	}
	printf("Thread %d sending start signal\n", my_rank);
	pthread_cond_signal(&startCond[my_rank]);

	printf("thread %d started\n", my_rank);
	pthread_mutex_unlock(&mutex);

	while(current_round < 3){

		pthread_mutex_lock(&mutex);
		startup++;

		if(startup == 3){
			printf("Player %d signaling for game to start\n", my_rank); fflush(stdout);
			pthread_cond_signal(&start_game);
		}

		pthread_cond_wait(&condArray[my_rank], &mutex);
		
		draw = pop();
		printf("Player%d hand: %d\n",my_rank, player_hand[my_rank]);
		printf("Player%d draw: %d\n",my_rank, draw);

		if(draw == player_hand[my_rank]){
			printf("WINNER\n");
			printf("Ending Round %d\n", current_round);
			push_back(player_hand[my_rank]);
			push_back(draw);
			current_round++;
		}else{
			if(rand() % 2 == 1){
				printf("RETURNING CARD %d\n", player_hand[my_rank]);
				push_back(player_hand[my_rank]);
				player_hand[my_rank] = draw;
			}else{
				printf("RETURNING CARD %d\n", draw);
				push_back(draw);
			}
		}
		
		if(my_rank == 3){
			signal_next = 0;
			current_round++;
			pthread_cond_signal(&condArray[0]);
		}
		
		if(my_rank == 3 && dealer_ready == 0)
			pthread_cond_wait(&condArray[0], &mutex);

		pthread_cond_signal(&condArray[0]);

		pthread_mutex_unlock(&mutex);

	}
	printf("PLAYER %d EXITING\n", my_rank);
	pthread_exit(NULL);
}


int main(int argc, char* argv[]){
	
	int i;
	long seed = 0;
	long thread;

	if(argc == 2){
		seed = atoi( argv[1] );
		printf("using seed: %d\n", seed);
	}else{
		seed = 56789;
		printf("using default seed: %d\n", seed);
	}
	
	if( pthread_mutex_init(&mutex, NULL) != 0){
		printf("Mutex Init Failed\n");
		return 1;
	}

	if(seed == 1){

		generateDeque();

		for(i = 0; i<100; i++){
			displayDeque();
			printf("pushing value %d\n", i);
			push_back(i);
			displayDeque();
			pop();
		}
	
	}
	
	//generate deck
	generateDeque();

	
	//I need a thread handle for the dealer and 3 players
	pthread_t* thread_handles = malloc( 4 * sizeof(pthread_t) );
	

	for(i = 0; i < 4; i++){
		pthread_cond_init(&condArray[i], NULL);
		pthread_cond_init(&startCond[i], NULL);
	}
	pthread_cond_init(&start_game, NULL);

	//create dealer and players
	for(thread = 1; thread<=3; thread++)
		pthread_create( &thread_handles[thread], NULL, player1, (void*)thread);
	
	pthread_create( &thread_handles[0], NULL, dealer, (void*)seed);

	//join players
	for(thread = 1; thread < 4; thread++)
		pthread_join( thread_handles[thread], NULL); 

	//join dealer
	pthread_join( thread_handles[0], NULL );

	for(i = 0; i < 4; i++){
		pthread_cond_destroy(&condArray[i]);
		pthread_cond_destroy(&startCond[i]);
	}
	pthread_cond_destroy(&start_game);
	pthread_mutex_destroy(&mutex);	
	destroyDeque();		
	
	return 0;
}




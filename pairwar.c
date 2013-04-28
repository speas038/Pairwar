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
int startArray[4] = {0,0,0,0};
int current_round = 0;
int first_player = 1;
int last_player = 3;
int current_player = 0;

struct Deque* head = NULL;
struct Deque* tail = NULL;
int winner = 0;
int player_hand[4];
int draw;

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

	nodePtr = head;

	s = dequeSize() - 2;
	srand(seed);

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

	nodePtr = head;
	while(nodePtr->next != NULL)
		nodePtr = nodePtr->next;
	tail = nodePtr;
}


int signal_next(){
	
	if( current_player == 0 ){
		if( current_round == 4 )
			return 3;
		else
			return current_round;

	}else if( current_player == 1 ){
		if( current_round == 2 )
			return 0;
		else 
			return 2;

	}else if( current_player == 2){
		if( current_round == 3 )
			return 0;
		else 
			return 3;

	}else if( current_player == 3 ){
		if( current_round == 1 )
			return 0;
		else 
			return 1;
	}
}

void dealer_turn(){
	
	
	printf("Dealer turn\n");

}

void player_turn(){
	
	
	printf("Player%d round %d\n", current_player, current_round);

}

void* dealer(void* seed){

	pthread_mutex_lock(&mutex);

	while(current_round < 3){

		if(startArray[1] != 1){
			pthread_cond_wait(&startCond[1], &mutex);
		}
		if(startArray[2] != 1){
			pthread_cond_wait(&startCond[2], &mutex);
		}
		if(startArray[3] != 1){
			pthread_cond_wait(&startCond[3], &mutex);
		}

		current_player = 0;

		current_round++;

		dealer_turn();

		pthread_cond_signal(&condArray[signal_next()]);
		pthread_cond_wait(&condArray[0], &mutex);	
	}
	current_round++;
	printf("DEALER EXITING\n"); 
	pthread_cond_signal(&condArray[signal_next()]);
	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
}

void* player(void* val){

	long my_rank = (long)val;

	pthread_mutex_lock(&mutex);

	while(current_round <= 3){

		startArray[my_rank] = 1;
		pthread_cond_signal(&startCond[my_rank]);

		current_player = my_rank;

		if(current_round > 0){


			player_turn();
	
			pthread_cond_signal(&condArray[signal_next()]);
		}
		pthread_cond_wait(&condArray[my_rank], &mutex);
	}
	current_player = my_rank;
	pthread_cond_signal(&condArray[signal_next()]);
	printf("PLAYER %d EXITING\n", my_rank, signal_next());
	pthread_mutex_unlock(&mutex);
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

	//this code is for testing purposes
	if(seed == 1){

		generateDeque();

		for(i = 0; i<100; i++){
			displayDeque();
			printf("deque size: %d\n", dequeSize());
			printf("pushing value %d\n", i);
			push_back(i);
			displayDeque();
			pop();
		}
	
	}

	
	if( pthread_mutex_init(&mutex, NULL) != 0){
		printf("Mutex Init Failed\n");
		return 1;
	}

	//I need a thread handle for the dealer and 3 players
	pthread_t* thread_handles = malloc( 4 * sizeof(pthread_t) );
	
	for(i = 0; i < 4; i++){
		pthread_cond_init(&condArray[i], NULL);
		pthread_cond_init(&startCond[i], NULL);
	}

	//create dealer and players
	for(thread = 1; thread<=3; thread++)
		pthread_create( &thread_handles[thread], NULL, player, (void*)thread);
	
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
	pthread_mutex_destroy(&mutex);	
	
	return 0;
}




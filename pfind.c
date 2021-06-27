#define _DEFAULT_SOURCE 
#define _OPEN_THREADS
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


int cnt_find_files=0;
int cnt_run_thread=0;
int cnt_error_thread=0;
pthread_t* thread;
pthread_mutex_t  lock;
pthread_mutex_t  lock_reddir;
pthread_mutex_t  lock_finish;
pthread_cond_t not_empty;
char* term;
int num_threads;
int someone_finish;

//===============================================
//Name: Oryan Tamsut
//Id: 324175223
//================================================


//==================queue manager================
typedef struct node 
{

	struct node* right;
	char* dir;
	struct node* left;

}node;

typedef struct queue 
{
	node* head;
	node* tail;
	int length;
}queue;

queue* my_queue;

queue* create_empty_queue() 
{
	queue* my_queue = (queue*)malloc(sizeof(queue));
	
	if (!my_queue) {perror("error malloc");exit(1);}
	
	my_queue->length = 0;
	my_queue->head = NULL;
	my_queue->tail = NULL;
	
	return my_queue;
}


void push(queue* my_queue, node* new_node) 
{	
	pthread_mutex_lock(&lock);
	if (my_queue->length == 0) 
	{
		my_queue->head = new_node;
		my_queue->tail = new_node;
	}
	
	else 
	{
		new_node->right = my_queue->tail;
		my_queue->tail->left = new_node;
		my_queue->tail = new_node;
	}
	
	my_queue->length++;
	pthread_cond_signal(&not_empty);
	pthread_mutex_unlock(&lock);
}


node* pop(queue* my_queue) 
{
	pthread_mutex_lock(&lock);
	node* head_node = my_queue->head;
	__sync_fetch_and_add(&cnt_run_thread, 1);

	if(my_queue->length==0)
	{
		 __sync_fetch_and_sub(&cnt_run_thread, 1);
		pthread_mutex_unlock(&lock);
		return NULL;
	}
	
	if(my_queue->length==1)
	{
		my_queue->head = NULL;
		my_queue->tail = NULL;
	}
	
	else
	{
		my_queue->head = my_queue->head->left;
		my_queue->head->right = NULL;
	}
	
	my_queue->length--;
	pthread_mutex_unlock(&lock);
	return head_node;
}

void finish(pthread_t tid)
{
	for(int i=0; i<num_threads; i++)
	{
		if(thread[i]!=tid) 
			pthread_cancel(thread[i]);
	}
	return;
}


void handler_cancel(void *arg)
{
    pthread_mutex_t *pm = (pthread_mutex_t *)arg;
    pthread_mutex_unlock(pm);
}


void handler_sigint(){
	finish(-1);
	node* root;
	free(thread);
	while(my_queue->length!=0)
	{
		root=pop(my_queue);
		free(root->dir);
		free(root);
	}
	free(my_queue);
	pthread_mutex_destroy(&lock_reddir);
	pthread_mutex_destroy(&lock);
	pthread_mutex_destroy(&lock_finish);
	pthread_cond_destroy(&not_empty);
	printf("Search stopped, found %d files\n",cnt_find_files);
	pthread_exit(NULL);
}


//=================find functions=====================
void* find_in_dir ()
{
	
    pthread_cleanup_push(handler_cancel, (void *)&lock);
    signal(SIGINT,handler_sigint);
	
	while(1){
	pthread_mutex_lock(&lock);
	
	while (my_queue->length==0){
		if(cnt_run_thread==0) 
		{	
			
			pthread_mutex_unlock(&lock);
			pthread_mutex_lock(&lock_finish);
			if(someone_finish>=1) 
			{
				pthread_mutex_unlock(&lock_finish);
				pthread_exit(NULL);
			}
			else
			{
				someone_finish=1;
				pthread_mutex_unlock(&lock_finish);
				finish(pthread_self());
				pthread_exit(NULL);
			}
		}
		else 
		{
		pthread_cond_wait(&not_empty,&lock);
		pthread_mutex_unlock(&lock);
		pthread_testcancel();
		}
	}
		pthread_mutex_unlock(&lock);
		pthread_testcancel();
		node* root;
		root=pop(my_queue);
		if(!root) continue;
		struct dirent *de; 
		char* ind_term;
		char* current_path;
		
	    
	    DIR *dr = opendir(root->dir); 
	   
	    
	    if (dr == NULL)
	    { 
	    	
	       __sync_fetch_and_sub(&cnt_run_thread, 1);
	       perror("error open directory");
	       __sync_fetch_and_add(&cnt_error_thread, 1);
		   pthread_exit( NULL);
	    } 
	    
	    pthread_mutex_lock(&lock_reddir);
		de = readdir(dr);
		pthread_mutex_unlock(&lock_reddir);
	
	    while (de)
	    {
	  
	    	current_path=(char*)malloc(sizeof(char)*(strlen(root->dir)+strlen(de->d_name)+2));
	    	strcpy(current_path,root->dir);
			strcpy(current_path+strlen(root->dir),"/");
			strcpy(current_path+strlen(root->dir)+1,de->d_name);
	    	
	    	if(de->d_type==DT_DIR)
	    	{
	    		if( strcmp(de->d_name,"..")==0|| strcmp(de->d_name,".")==0)
	    		 {
	    			free(current_path);
	    			pthread_mutex_lock(&lock_reddir);
					de = readdir(dr);
					pthread_mutex_unlock(&lock_reddir);
	    			continue;
	    		}
	    		
	    		node* new_dir=(node*)malloc(sizeof(node));
	    		
	    		if (!new_dir) 
	    		{
	    			
	    			__sync_fetch_and_sub(&cnt_run_thread, 1);
	    			perror("error malloc");
	    			__sync_fetch_and_add(&cnt_error_thread, 1);
					pthread_exit(NULL);
	    		}
	    
	    		new_dir->dir=current_path;
	    		push(my_queue,new_dir);
	    	}
	    	
	    	else
	    	{
	    		ind_term=(char*)strstr(de->d_name,term);
	    		if(ind_term)
	    		{
	    			printf("%s \n",current_path);
	    			__sync_fetch_and_add(&cnt_find_files, 1);
	    		
	    		}
	    	}
	    
		    pthread_mutex_lock(&lock_reddir);
			de = readdir(dr);
			pthread_mutex_unlock(&lock_reddir);
		    
	      }
	      __sync_fetch_and_sub(&cnt_run_thread, 1);
	      free(root->dir);
		  free(root);
		  closedir(dr); 
    }
 
    pthread_cleanup_pop(1);
    return (void*)NULL; 
} 



//=========main=================
int main(int argc, char* argv[]) 
{
	if(argc<4) {perror("Not enough arguments \n");exit(1);}
	char* root_path=argv[1];
	term= argv[2];
	num_threads=atoi(argv[3]);
	int rc;
	
	DIR *dr = opendir(root_path); 
	if (dr == NULL){perror("error open directory- not valid directory");exit(1);}

	if (num_threads<=0) {perror("not valid num threads");exit(1);} 
	
	rc = pthread_mutex_init( &lock, NULL );
	if(rc!=0) {perror("ERROR in pthread_mutex_init() \n");exit(1);}
	
	pthread_mutex_init( &lock_finish, NULL );
	if( rc!=0 ) {perror("ERROR in pthread_mutex_init() \n");exit(1);}
	
	 rc = pthread_mutex_init( &lock_reddir, NULL );
	if( rc!=0 ) {perror("ERROR in pthread_mutex_init() \n");exit(1);}
	 
	rc= pthread_cond_init (&not_empty, NULL);
	if( rc!=0 ) {perror("ERROR in pthread_cond_init() \n");exit(1);}
	
	my_queue=create_empty_queue();
	if(!my_queue){perror("error malloc queue");exit(1);}
	
	node* new_dir=(node*)malloc(sizeof(node));
	if (!new_dir){perror("error malloc root directory");exit(1);}
	
	new_dir->dir=(char*)malloc(sizeof(char)*(strlen(root_path)+1));
	if (!new_dir->dir) {perror("error malloc root directory path");exit(1);}
	
	strcpy(new_dir->dir,root_path);
	
	push(my_queue,new_dir);
	
	thread=(pthread_t*)malloc(sizeof(pthread_t)*num_threads);
	if (!thread){perror("error malloc thread array");exit(1);}
	
	for(int i=0; i<num_threads; i++)
	{
	
		rc = pthread_create(&thread[i],NULL,find_in_dir,NULL); 
		if (rc!=0){perror("ERROR in pthread_create()\n");exit( 1 );}
	}
	
	for( int i = 0; i < num_threads; i++ ) 
  	{
	    rc = pthread_join(thread[i], NULL);
	    if (rc!=0) {perror("ERROR in pthread_join()");exit( 1 );}
  	}
	
	printf("Done searching, found %d files \n",cnt_find_files);
	free(thread);
	free(my_queue);
	pthread_mutex_destroy(&lock_reddir);
	pthread_mutex_destroy(&lock);
	pthread_mutex_destroy(&lock_finish);
	pthread_cond_destroy(&not_empty);
	if(cnt_error_thread==num_threads) exit (1);
  	
  	exit(0);
}








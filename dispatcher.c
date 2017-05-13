#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#define SOCK_PATH "/tmp/.muffins_socket"

struct socket_list {
    int s;
    struct socket_list *next;
};

/**	add_to_list() is responsible for adding the socket numbers to a linked list
*/
struct socket_list * add_to_list(struct socket_list * root, int socket);

/**	broadcast_message() iterates through all the sockets in the socket list
*       and sends the str in the argument to all the sockets using send
*/
void broadcast_message(struct socket_list * root, char * str);

/**	write_message() is a threaded function which loops through asking for input
*       from stdin, it sends the string to pointer used by broadcast
*/
void *write_message(void *ptr);

/**	deconstruct_socket_list() goes through the linked list and closes the sockets
*       as well as free's the memory
*/
void deconstruct_socket_list(struct socket_list* root);

/**	accept_connections() is a threaded function which makes socket connections and adds
*       them to the linked list
*/
void *accept_connections(void *ptr);

int main(void)
{

    //Initializing the pointers used for the socket linked list
    struct socket_list * clients = malloc(sizeof(struct socket_list));
    clients->next = NULL;
    clients->s = 0;

    //Starting the write_message thread for the server
    pthread_t thread1;
    int iret1;

    iret1 = pthread_create( &thread1, NULL, write_message, (void *) clients);
    if(iret1) {
        perror("Error - pthread_create\n");
        exit(1);
    }

    //Starting the accept_connections thread for the server
    pthread_t thread2;
    int iret2;

    iret2 = pthread_create(&thread2, NULL, accept_connections, (void *) clients);
    if(iret2) {
        perror("Error - pthread_create on 2\n");
        exit(1);
    }

    //Waits till the write_message thread is finished
    pthread_join(thread1, NULL);

    //Forcibly exits the accept connections thread
    pthread_cancel(thread2);

    //Runs through the linked lists and frees up memory
    deconstruct_socket_list(clients);

    free(clients);
    pthread_exit(NULL);

}

struct socket_list * add_to_list(struct socket_list * root, int socket)
{
    struct socket_list * newNode = malloc(sizeof(struct socket_list));
    newNode->s = socket;
    newNode->next = NULL;

    if(root == NULL) {
        root = newNode;                
    } else {
        struct socket_list * cursor = root;
        while (cursor->next != NULL)
        {
            cursor = cursor->next;
        }
        cursor->next = newNode;
    }

    return root;
}

void broadcast_message(struct socket_list * root, char * str)
{
    struct socket_list * cursor = root;
    while(cursor)
    {
        send(cursor->s, str, strlen(str), 0);
        cursor = cursor->next;
    }
}

void *write_message(void *ptr)
{
    struct socket_list * clients = (struct socket_list *)ptr;

    char * line = NULL;
    char msg[100];
    while(true)
    {

        memset(msg, '\0', sizeof(msg));
        line = fgets(msg, sizeof(msg), stdin);
        if(!line) {
            break;
        }
        if(clients) {
            broadcast_message(clients, msg);
        }
    }

    pthread_exit(NULL);
}

void deconstruct_socket_list(struct socket_list* root)
{
    struct socket_list * cursor;

    while(root != NULL)
    {
        cursor = root;
        root = root->next;
        close(cursor->s);
        free(cursor);
    }   
}

void *accept_connections(void *ptr)
{
    int s=-1, s2=-1;
    struct sockaddr_un local, remote;
    int len, return_val, t;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if(s < 0) {
        perror("Socket()\n");
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);

    return_val = bind(s, (struct sockaddr *)&local, len);
    if(return_val < 0) {
        perror("bind()\n");
        exit(1);
    }

    //Replace 5 with actual named value later
    return_val = listen(s, 5);
    if(return_val < 0) {
        perror("listen()\n");
        exit(1);
    }

    struct socket_list * clients = (struct socket_list *)ptr;

    for(;;)
    {
		t = sizeof(remote);
		if((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
			perror("accept");
			exit(1);
		}

        clients = add_to_list(clients, s2);

    }
    pthread_exit(NULL);
}


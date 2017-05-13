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

#define SOCK_PATH "bs_socket"

struct socket_list {
    int s;
    struct socket_list *next;
};

volatile bool got_EOF = false;

void signal_handler(int signal);
struct socket_list * add_to_list(struct socket_list * root, int socket);
void broadcast_message(struct socket_list * root, char * str);
void *write_message(void *ptr);
void deconstruct_socket_list(struct socket_list* root);

int main(void)
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

    struct socket_list * clients = malloc(sizeof(struct socket_list));
    clients->next = NULL;

    pthread_t thread1;
    int iret1;

    iret1 = pthread_create( &thread1, NULL, write_message, (void *) clients);
    if(iret1)
    {
        perror("Error - pthread_create\n");
        exit(1);
    }

    for(;;) {
        if(got_EOF)
        {
            break;
        }

		printf("Waiting for a connection...\n");
		t = sizeof(remote);
		if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
			perror("accept");
			exit(1);
		}

        clients = add_to_list(clients, s2);

    }

    pthread_join(thread1, NULL);
    //Make sure you get closes or shutdowns working here
    deconstruct_socket_list(clients);
    //Need to write a function that goes through the linked lists
    pthread_exit(NULL);

}

struct socket_list * add_to_list(struct socket_list * root, int socket)
{
    struct socket_list * newNode = malloc(sizeof(struct socket_list));
    newNode->s = socket;
    newNode->next = NULL;

    if (root == NULL)
    {
        root = newNode;                
    }
    else
    {
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
    while (cursor)
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
    while(1) {

        memset(msg, '\0', sizeof(msg));
        line = fgets(msg, sizeof(msg), stdin);
        if(!line)
        {
            got_EOF = true;
            break;
        }
        broadcast_message(clients, msg);
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

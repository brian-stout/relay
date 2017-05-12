#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#define SOCK_PATH "bs_socket"
#define NUM_THREAD 2

struct socket_list {
    int s;
    struct socket_list *next;
};

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

    char msg[100];
    while(1) {
        memset(msg, '\0', sizeof(msg));
        fgets(msg, sizeof(msg), stdin);
        broadcast_message(clients, msg);
    }

    pthread_exit(NULL);
}

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

    pthread_t thread1;
    int iret1;

    iret1 = pthread_create( &thread1, NULL, write_message, (void *) clients);
    if(iret1)
    {
        perror("Error - pthread_create\n");
        exit(1);
    }

    for(;;) {
		printf("Waiting for a connection...\n");
		t = sizeof(remote);
		if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
			perror("accept");
			exit(1);
		}

        clients = add_to_list(clients, s2);

    }

    //Make sure you get closes or shutdowns working here   
    //Need to write a function that goes through the linked lists
    pthread_exit(NULL);

}
        

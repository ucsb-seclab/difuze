#include<stdio.h>
#include<string.h>	  //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>	  //write
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdbool.h>

// Socket boilerplate code taken from here: http://www.binarytides.com/server-client-example-c-sockets-linux/

/*
 seed, ioctl_id, num_mappings, num_blobs, dev_name_len, dev_name, map_entry_t_arr, blobs
*/
int debug = 0;

typedef struct {
	int src_id;
	int dst_id;
	int offset;
} map_entry_t;

short tiny_vals[18] = {128, 127, 64, 63, 32, 31, 16, 15, 8, 7, 4, 3, 2, 1, 0, 256, 255, -1};
int *small_vals;
int num_small_vals;

// populates small_vals when called
void populate_arrs(int top) {
	int num = 1;
	int count = 0;
	while (num < top) {
		//printf("%d\n", num);
		num <<= 1;
		count += 2;
	}
	// top
	count += 1;
	// -1
	count += 1;
	num_small_vals = count;
	num >>= 1;

	small_vals = malloc(sizeof(int)*count);
	memset(small_vals, 0, count);

	int i = 0;
	while(num > 1) {
		small_vals[i] = num;
		i++;
		small_vals[i] = num-1;
		i++;
		num >>= 1;
	}
	small_vals[i] = 0;
	small_vals[i+1] = top;
	small_vals[i+2] = top-1;
	small_vals[i+3] = -1;
}

// generate a random value of size size and store it in elem.
// value has a weight % chance to be a "small value"
void gen_rand_val(int size, char *elem,  int small_weight) {
	int i;

	if ((rand() % 100) < small_weight) {
		// do small thing
		unsigned int idx = (rand() % num_small_vals);
		printf("Choosing %d\n", small_vals[idx]);
		switch (size) {
			case 2:
				idx = (rand() % 18);
				*(short *)elem = tiny_vals[idx];
				break;
			case 4:
				*(int *)elem = small_vals[idx];
				break;

			case 8:
				*(long long*)elem = small_vals[idx];
				break;

			default:
				printf("Damn bro. Size: %d\n", size);
				exit(-1);
		}
	}

	else {

		for(i=0; i < size; i++) {
			elem[i] = (char)(rand()%0x100);
		}
	}

}
 
int main(int argc , char *argv[])
{
	int num_blobs = 0, num_mappings = 0, i = 0, dev_name_len = 0, j;
	unsigned int ioctl_id = 0;
	char *dev_name;
	void *tmp;
	char **ptr_arr;
	int *len_arr;
	unsigned int seed;

	int sockfd , client_sock , c , read_size;
	struct sockaddr_in server , client;
	int msg_size;
	void *generic_arr[264];

	// max val for small_vals array
	int top = 8192;
	// chance that our generics are filled with "small vals"
	int default_weight = 50;
	populate_arrs(top);
	int retest = 0;
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return -1;
	}
	if (argc > 2) {
		retest = 1;
		goto rerun;
	}
	


	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if (sockfd == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
	 
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));

	//Bind
	if( bind(sockfd,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
listen:	 
	// Listen
	listen(sockfd , 3);
	 
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	 
	// accept connection from an incoming client
	client_sock = accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");
	 
	msg_size = 0;
	// Receive a message from client
	while( (read_size = recv(client_sock , &msg_size , 4 , 0)) > 0 )
	{
		// recv the entire message
		char *recv_buf = calloc(msg_size, sizeof(char));
		if (recv_buf == NULL) {
			printf("Failed to allocate recv_buf\n");
			exit(-1);
		}

		int nrecvd = recv(client_sock, recv_buf, msg_size, 0);
		if (nrecvd != msg_size) {
			printf("Error getting all data!\n");
			printf("nrecvd: %d\nmsg_size:%d\n", nrecvd, msg_size);
			exit(-1);
		}
		// quickly save a copy of the most recent data
		int savefd = open("/sdcard/saved", O_WRONLY|O_TRUNC|O_CREAT, 0644);
		if (savefd < 0) {
			perror("open saved");
			exit(-1);
		}

		int err = write(savefd, recv_buf, msg_size);
		if (err != msg_size) {
			perror("write saved");
			exit(-1);
		}
		fsync(savefd);
		close(savefd);
rerun:
		if (retest) {
			recv_buf = calloc(msg_size, sizeof(char));
			int fd = open("/sdcard/saved", O_RDONLY);
			if (fd < 0) {
				perror("open:");
				exit(-1);
			}
			int fsize = lseek(fd, 0, SEEK_END);
			printf("file size: %d\n", fsize);
			lseek(fd, 0, SEEK_SET);
			read(fd, recv_buf, fsize);
		}

		char *head = recv_buf;
		seed = 0;
		//seed, ioctl_id, num_mappings, num_blobs, dev_name_len, dev_name, map_entry_t_arr, blob_len_arr, blobs
		memcpy(&seed, head, 4);
		head += 4;
		memcpy(&ioctl_id, head, 4);
		head += 4;
		memcpy(&num_mappings, head, 4);
		head += 4;
		memcpy(&num_blobs, head, 4);
		head += 4;
		memcpy(&dev_name_len, head, 4);
		head += 4;
		
		// srand with new seed
		srand(seed);

		/* dev name */
		dev_name = calloc(dev_name_len+1, sizeof(char));
		if (dev_name == NULL) {
			printf("Failed to allocate dev_name\n");
			exit(-1);
		}
		memcpy(dev_name, head, dev_name_len);
		head += dev_name_len;

		/* map */
		map_entry_t *map = calloc(num_mappings, sizeof(map_entry_t));
		if (map == NULL) {
			printf("Failed to allocate map\n");
			exit(-1);
		}

		if (num_mappings != 0) {
			memcpy(map, head, num_mappings*sizeof(map_entry_t));
			head += num_mappings*sizeof(map_entry_t);
		}

		/* blobs */
		
		// first create an array to store the sizes themselves
		len_arr = calloc(num_blobs, sizeof(int));
		if (len_arr == NULL) {
			printf("Failed to allocate len_arr\n");
			exit(-1);
		}

		// we'll also want an array to store our pointers
		ptr_arr = calloc(num_blobs, sizeof(void *));
		if (ptr_arr == NULL) {
			printf("Failed to allocate ptr_arr\n");
			exit(-1);
		}


		// copy the blob sizes into our size_arr
		for (j=0; j < num_blobs; j++) {
			memcpy(&len_arr[j], head, sizeof(int));
			head += sizeof(int);
		}

		// we'll also want memory bufs for all blobs
		// now that we have the sizes, allocate all the buffers we need
		for (j=0; j < num_blobs; j++) {
			ptr_arr[j] = calloc(len_arr[j], sizeof(char));
			//printf("just added %p to ptr_arr\n", ptr_arr[j]);
			if (ptr_arr[j] == NULL) {
				printf("Failed to allocate a blob store\n");
				exit(-1);
			}

			// might as well copy the memory over as soon as we allocate the space
			memcpy((char *)ptr_arr[j], head, len_arr[j]);
			head += len_arr[j];
		}
		
		int num_generics = 0;

		// time for pointer fixup
		for (i=0; i < num_mappings; i++) {
			// get out entry
			map_entry_t entry = map[i];
			// pull out the struct to be fixed up
			char *tmp = ptr_arr[entry.src_id];
		
			// check if this is a struct ptr or just a generic one
			
			// just a generic one
			if (entry.dst_id < 0) {
				// 90% chance we fixup the generic
				if ( (rand() % 10) > 0) {
					int buf_len = 128;
					char *tmp_generic = malloc(buf_len);
					memset(tmp_generic, 0, buf_len);
					// 95% chance we fill it with data
					if ((rand() % 100) > 95) {
						// if dst_id is < 0, it's abs value is the element size
						int size = -1 * entry.dst_id;
						int weight;
						// if it's a char or some float, never choose a "small val"
						if (size == 1 || size > 8)
							weight = 0;
						else
							weight = default_weight;

						for (i=0; i < buf_len; i+=size) {
							gen_rand_val(size, &tmp_generic[i], weight);
						}
					}
					generic_arr[num_generics] = tmp_generic;
					memcpy(tmp+entry.offset, &tmp_generic, sizeof(void *));
					num_generics += 1;
					if (num_generics >= 264) {
						printf("Code a better solution for storing generics\n");
						exit(1);
					}
				}
			}

			// a struct ptr, so we have the data
			else {
				// 1 in 400 chance we don't fixup
				if ( (rand() % 400) > 0) {
					// now point it to the correct struct/blob
					// printf("placing %p, at %p\n", ptr_arr[entry.dst_id], tmp+entry.offset);
					memcpy(tmp+entry.offset, &ptr_arr[entry.dst_id], sizeof(void *));
				}
			}
		}
		
		if (debug) {
			printf("ioctl_id: %d\n", ioctl_id);
			printf("num_mappings: %d\n", num_mappings);
			printf("num_blobs: %d\n", num_blobs);
			printf("dev_name_len: %d\n", dev_name_len);
			printf("dev_name: %s\n", dev_name);
		}
		
		// time for the actual ioctl
		int fd = open(dev_name, O_RDONLY);
		if (fd < 0) {
			perror("open");
			exit(-1);
		}
		fflush(stdout);
		if ((ioctl(fd, ioctl_id, ptr_arr[0])) == -1)
			perror("ioctl");
		
		else
			printf("good hit\n");
		close(fd);

		if (retest)
			exit(0);

		fflush(stdout);
		// okay now free all the shit we alloced
		free(recv_buf);
		free(dev_name);
		if (map != NULL)
			free(map);
		free(len_arr);
		for (i=0; i < num_blobs; i++) {
			//printf("%d: free'ing %p\n", i, ptr_arr[i]);
			free(ptr_arr[i]);
		}
		free(ptr_arr);
		for (i=0; i < num_generics; i++) {
			free(generic_arr[i]);
		}
		
		write(client_sock, &msg_size, 4);

		msg_size = 0;
	}
	 
	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
		close(client_sock);
		goto listen;
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}
	 
	return 0;
}

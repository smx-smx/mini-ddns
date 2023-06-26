/**
 * @file monitor.c
 * @author Stefano Moioli (smxdev4@gmail.com)
 * @brief Runs a pre-defined command when the specified interface changes
 * @version 0.1
 * @date 2023-06-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>

#define MAX_PARTS 10

enum state {
	INITIAL = 0,
	LINK_UP,
	NEW_ROUTE
};

enum state state = INITIAL;

//#define STR_EQ(a, b) (strncmp((a), (b), strlen(b)) == 0)
#define STR_EQ(a, b) (strcmp((a), (b)) == 0)

char *last_ipaddr = NULL;

int main(int argc, char *argv[]){
	char buf[256];
	char *parts[MAX_PARTS];
	
	setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

	memset(buf, 0x00, sizeof(buf));
	memset(parts, 0x00, sizeof(parts));

	puts("interface: " INTERFACE_NAME);
	puts("command  : " COMMAND);

	FILE *cmd = popen("./ifwatch", "r");
	if(!cmd){
		fprintf(stderr, "popen() failed\n");
		return 1;
	}
	while(!feof(cmd)){
		fgets(buf, sizeof(buf), cmd);
		{
			char *nl = strrchr(buf, '\n');
			if(nl != NULL){
				*nl = 0x00;
			}
		}

		int i;
		char *tok;
		for(i=0, tok = strtok(buf, ":");
			tok != NULL && i < MAX_PARTS;
			tok = strtok(NULL, ":"), i++
		){
			parts[i] = tok;
			printf("tok[%d] = \"%s\"\n", i, parts[i]);
		}

		if(parts[0] == NULL){
			continue;
		}

		/*for(int i=0; i<MAX_PARTS; i++){
			printf("tok[%d] = %s\n", i, parts[i]);
		}*/

		switch(state){
			case INITIAL:
				if(STR_EQ(parts[0], "link") &&
				   STR_EQ(parts[1], "up") &&
				   STR_EQ(parts[2], INTERFACE_NAME)
				){
					puts("got link up");
					state = LINK_UP;
				}
				break;
			case LINK_UP:
				if(STR_EQ(parts[0], "route") &&
				   STR_EQ(parts[1], "add")
				){
					puts("got route");
					state = NEW_ROUTE;
				}
				break;
			case NEW_ROUTE:
				if(STR_EQ(parts[0], "ip") &&
				   STR_EQ(parts[1], "add")
				){
					puts("got new ip");
					printf("New IP: %s, invoking " COMMAND "\n", parts[3]);


					if(last_ipaddr != NULL && !strcmp(parts[3], last_ipaddr)){
						// abort update, same IP as the last one
						printf("aborting update, Old IP %s == New IP %s\n", last_ipaddr, parts[3]);
						break;
					}

					if(last_ipaddr != NULL){
						free(last_ipaddr); last_ipaddr = NULL;
					}
					last_ipaddr = strdup(parts[3]);

					pid_t pid = fork();
					if(pid == 0){
						execv("/bin/sh", (char *[]){
							"/bin/sh",
							COMMAND,
							parts[3],
							NULL
						});
						exit(0);
					}

					state = INITIAL;
				}
				break;
		}
	}
	pclose(cmd);
}
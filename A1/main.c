
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

int commandNumber = 0;
struct node *head_job = NULL;
struct node *current_job = NULL;

//Initiliaze the args(input) to null once the command has been processed
// this is to clear it to accept another command in the next while loop
void initialize(char *args[]);

int getcmd(char *line, char *args[], int *background) {
	int i = 0;
	char *token, *loc;

	//Copy the line to a new char array because after the tokenization a big part of the line gets deleted since the null pointer is moved
	char *copyCmd = malloc(sizeof(char) * sizeof(line) * strlen(line));
	sprintf(copyCmd, "%s", line);

	// Check if background is specified..
	if ((loc = index(line, '&')) != NULL) {
		*background = 1;
		*loc = ' ';
	} else
		*background = 0;

	//Create a new line pointer to solve the problem of memory leaking created by strsep() and getline() when making line = NULL
	char *lineCopy = line;
	while ((token = strsep(&lineCopy, " \t\n")) != NULL) {
		for (int j = 0; j < strlen(token); j++)
			if (token[j] <= 32)
				token[j] = '\0';
		if (strlen(token) > 0)
			args[i++] = token;
	}

	return i;
}

void initialize(char *args[]) {
	for (int i = 0; i < 20; i++) {
		args[i] = NULL;
	}
	return;
}

struct node {
	int number; // the job number
	int pid; // the process id of the a specific process
	struct node *next; // when another process is called you add to the end of the linked list
};

/* Add a job to the list of jobs
 */
void addToJobList(int process_pid) {

	struct node *job = malloc(sizeof(struct node));

	//If the job list is empty, create a new head
	if (head_job == NULL) {
		job->number = 1;
		job->pid = process_pid;

		//the new head is also the current node
		job->next = NULL;
		head_job = job;
		current_job = head_job;
	}

	//Otherwise create a new job node and link the current node to it
	else {

		job->number = current_job->number + 1;
		job->pid = process_pid;

		current_job->next = job;
		current_job = job;
		job->next = NULL;
	}
}

int runJob(char *args[], int background) {
    pid_t  pid;     // this is just a type def for int really..
    pid = fork();
    if (pid == 0) {
        //Child process..
        int w = rand() % 10;
		sleep(w);
        execvp(args[0], args);
    } else {
        int status;
        if(background) {
            printf("Running %d in background\n", pid);
            addToJobList(pid);
        } else {
            waitpid(pid, &status, WUNTRACED);
        }
    }
    return pid;
}

#define for_each_item(item, previous, head) \
    for(item = head; item != NULL; previous = item, item = item->next)

void linkedForEach(struct node *pNode, void (*fn)()) {
	struct node *pPrevious = NULL;
	while(pNode != NULL) {
		fn(pNode, pPrevious);
		pPrevious = pNode;
		pNode = pNode->next;
	}
}

void linkedSlice(struct node *pNode, struct node *pPrevious, struct node **pHead) {
    if(pPrevious != NULL) {
        pPrevious->next = pNode->next;
    } else {
		*pHead = (pNode->next) ? pNode->next : NULL;
    }
	free(pNode);
}

void sliceInactiveJobs(struct node *pNode, struct node *pPrevious) {
    if(kill(pNode->pid,0) != 0) {
        linkedSlice(pNode, pPrevious, &head_job);
    }
}

void printJob(struct node *pNode) {
    printf("[%d] \t PID : %ld\n", pNode->number,(long) pNode->pid);
}

int main(void) {
	char *args[20];
	int bg;

	char *user = getenv("USER");
	if (user == NULL) user = "User";

	char str[sizeof(char)*strlen(user) + 4];
	sprintf(str, "\n%s>> ", user);

    time_t now;
    srand((unsigned int) (time(&now)));


	while (1) {
		initialize(args);
		bg = 0;

		int length = 0;
		char *line = NULL;
		size_t linecap = 0; // 16 bit unsigned integer
		sprintf(str, "\n%s>> ", user);
		printf("%s", str);

		length = getline(&line, &linecap, stdin);
		if (length <= 0) { //if argument is empty
			exit(-1);
		}
		int cnt  = getcmd(line, args, &bg);
		if (!strcmp("ls", args[0])) { // returns 0 if they are equal , then we negate to make the if statment true
			runJob(args, bg);
		} else if (!strcmp("cat", args[0])) { // returns 0 if they are equal , then we negate to make the if statment true
			runJob(args, bg);
		} else if (!strcmp("cd", args[0])) {

			int result = 0;
			if (args[1] == NULL) { // this will fetch home directory if you just input "cd" with no arguments
				char *home = getenv("HOME");
				if (home != NULL) {
					result = chdir(home);
				} else {
					printf("cd: No $HOME variable declared in the environment");
				}
			}
			//Otherwise go to specified directory
			else {
				result = chdir(args[1]);
			}
			if (result == -1) fprintf(stderr, "cd: %s: No such file or directory", args[1]);

		} else if(strcasecmp(args[0],"jobs")==0 && cnt == 1) {

            linkedForEach(head_job, sliceInactiveJobs);
            linkedForEach(head_job, printJob);

            continue;
        } else if (strcasecmp(args[0],"fg")==0 && cnt == 2) {
            int jobid = atoi(args[1]);

            struct node *cur = NULL;
            struct node *prev = NULL;
            for_each_item(cur, prev, head_job) {
                if(cur->number == jobid) {
                    kill(cur->pid,SIGCONT);		//bring the process to the foreground
                    int status;
                    waitpid(cur->pid,&status,WUNTRACED);
                }
            }
            continue;
        }

		free(line);
	}
}

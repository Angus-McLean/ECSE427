
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

struct node {
    int number; // the job number
    int pid; // the process id of the a specific process
    struct node *next; // when another process is called you add to the end of the linked list
};

struct jobParams {
    char *args[20];
    int bg;
    int redirectFlag;
    char *redirectTo;
};

struct node *head_job = NULL;
struct node *current_job = NULL;

// store the current forground job.
int ACTIVE_JOB;

// clears args array, resets jobParams struct
void initialize(char *args[], struct jobParams *job) {
	for (int i = 0; i < 20; i++) {
		args[i] = NULL;
	}

    *(job->args) = *args;
    job->bg = 0;
    job->redirectFlag = 0;
    job->redirectTo = NULL;

    return;
}

// takes line<string> and args+nextJob pointers, parses the line into arguments and sets nextJob params accordingly
int getcmd(char *line, char *args[], struct jobParams *nextJob) {
	int i = 0;
	char *token, *loc;

	//Copy the line to a new char array because after the tokenization a big part of the line gets deleted since the null pointer is moved
	char *copyCmd = malloc(sizeof(char) * sizeof(line) * strlen(line));
	sprintf(copyCmd, "%s", line);

	// Check if background is specified..
	if ((loc = index(line, '&')) != NULL) {
        nextJob->bg = 1;
		*loc = ' ';
	} else {
        nextJob->bg = 0;
    }

    if (strstr(line, ">>") != NULL) {
        nextJob->redirectFlag = O_APPEND;
    } else if (strstr(line, ">") != NULL) {
        nextJob->redirectFlag = O_TRUNC;
    }

	//Create a new line pointer to solve the problem of memory leaking created by strsep() and getline() when making line = NULL
	char *lineCopy = line;
	while ((token = strsep(&lineCopy, " \t\n")) != NULL) {
		for (int j = 0; j < strlen(token); j++)
			if (token[j] <= 32)
				token[j] = '\0';
		if (strlen(token) > 0)
			args[i++] = token;
	}

    if(nextJob->redirectFlag != NULL) {
        nextJob->redirectTo = args[i-1];
        args[i-2] = NULL;
        args[i-1] = NULL;
    }

	return i;
}

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

// takes args and nextJob Params and runs the job, saves to linked list and handles output redirection if required
int runJob(char *args[], struct jobParams *nextJob) {
    pid_t  pid;     // this is just a type def for int really..
    pid = fork();
    if (pid == 0) {
        //Child process..
        if(nextJob->redirectTo != NULL) {
            close(1);
            open(nextJob->redirectTo, O_WRONLY | O_CREAT | nextJob->redirectFlag, 0755);
        }
        int w = rand() % 10;
        sleep(w);
        execvp(args[0], args);
        exit(0);
    } else {
        int status;
        if(nextJob->bg) {
            printf("Running %d in background\n", pid);
            addToJobList(pid);
        } else {
            ACTIVE_JOB = pid;
            waitpid(pid, &status, WUNTRACED);
        }
    }
    return pid;
}

#define for_each_item(item, previous, head) \
    for(item = head; item != NULL; previous = item, item = item->next)

// slices a node from the linked list.
void linkedSlice(struct node *pNode, struct node *pPrevious, struct node **pHead) {
    if(pPrevious != NULL) {
        pPrevious->next = pNode->next;
    } else {
		*pHead = (pNode->next) ? pNode->next : NULL;
    }
	free(pNode);
}

static void signalHandler(int sig) {
    if (sig == SIGTSTP) {
        printf("Ahhhh it cannot be stopped!\n");
    }
    if (sig == SIGINT) {
        if(ACTIVE_JOB != NULL && kill(ACTIVE_JOB,0) == 0) {
            // active job is still processing. Kill it.
            printf("Killing active process\n");
            kill(ACTIVE_JOB, SIGTERM);
        }
    }
}

int main(void) {
    // used to avoid having zombie child processes
    signal(SIGCHLD, SIG_IGN);

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

  	char *args[20];
    struct jobParams nextJob = {*args, 0, 0, NULL};

  	char *user = getenv("USER");
  	if (user == NULL) user = "User";

  	char str[sizeof(char)*strlen(user) + 4];
  	sprintf(str, "\n%s>> ", user);

    time_t now;
    srand((unsigned int) (time(&now)));

    // handle user signals
    if(signal(SIGINT, signalHandler) == SIG_ERR) {
        printf("ERROR! Couldn't bind signal handler\n");
        exit(1);
    }
    // handle user signals
    if(signal(SIGTSTP, signalHandler) == SIG_ERR) {
        printf("ERROR! Couldn't bind signal handler\n");
        exit(1);
    }

	while (1) {
		initialize(args, &nextJob);

		int length = 0;
		char *line = NULL;
		size_t linecap = 0; // 16 bit unsigned integer
		sprintf(str, "\n%s>> ", user);
		printf("%s", str);

		length = getline(&line, &linecap, stdin);
		if (length <= 1) { //if argument is empty
            printf("Empty input, exiting.\n");
            exit(-1);
		}
		int cnt  = getcmd(line, args, &nextJob);
		if (!strcmp("ls", args[0])) {
			runJob(args, &nextJob);
		} else if (!strcmp("cat", args[0])) {
			runJob(args, &nextJob);
		} else if (!strcmp("cp", args[0])) {
			runJob(args, &nextJob);
		} else if (!strcmp("exit", args[0])) {
			exit(-1);
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

            struct node *cur = NULL;
            struct node *prev = NULL;
            for_each_item(cur, prev, head_job) {
                if(kill(cur->pid,0) != 0) {
                    linkedSlice(cur, prev, &head_job);
                } else {
                    printf("[%d] \t PID : %ld\n", cur->number,(long) cur->pid);
                }
            }
        } else if (strcasecmp(args[0],"fg")==0 && cnt == 2) {
            int jobid = atoi(args[1]);

            struct node *cur = NULL;
            struct node *prev = NULL;
            for_each_item(cur, prev, head_job) {
                if(cur->number == jobid) {
                    kill(cur->pid, SIGCONT);		//bring the process to the foreground
                    int status;
                    ACTIVE_JOB = cur->pid;
                    waitpid(cur->pid,&status,WUNTRACED);
                }
            }
        }

		free(line);
	}
}

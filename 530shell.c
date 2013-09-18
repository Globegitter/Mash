/*
 * Author: Markus Padourek
 * Date: 08/09/2013 (dd/mm/yy)
 * Mash - v0.6.0
 * Your simple and friendly shell-servant
 *
 * A simple Shell for Unix, that can execute any valid file, 
 * as well as files/commands in PATH. When it executes a file/command
 * it creates a child-Process which can be terminated via CTRL+C. Once
 * it has executed the command, it returns to your current work directory
 * and is simply waiting for the next command.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

//Different colors to print text
#define CCYAN  "\x1b[36m"
#define CRED     "\x1b[31m"
#define CRESET   "\x1b[0m"

#define LINE_LENGTH 1024
#define MAX_ARGS 15
#define MAX_SUSPENDS 5

time_t now, end;
char* username;
char cwd[LINE_LENGTH];
int childPID = -1;

void errorLogger(int errLevel, char* err){
	fprintf(stderr, "%s", err);
}

/*
 * Signal handler for CTRL+Z
 * Should do nothing in parent-process if there is a child-process
 * The default signal behaviour should still work for the child
 */
void  tstpHandler(int sig){
	return;
	printf("Suspending yooouuu: %i\n", childPID);
	if(childPID == -1){
    	signal(sig, SIG_DFL);
    	raise(SIGTSTP);
    	signal(sig, tstpHandler);
     }else{
     	printf("%s, suspending process with PID %i.", username, childPID);
     }
}

/*
 * Signal handler for CTRL+C
 * Should do nothing in parent-process if there is a child-process
 * The default signal behaviour should still work for the child
 */
void interruptHandler(int sig){
	if(childPID == -1){
    	signal(sig, SIG_DFL);
    	raise(SIGINT);
     }
}

/*
 * Prompts one-time Welcome message.
 * Should display username and current time.
 */
void promptWelcomeMsg(){
	printf("Great to have you back %s. Your Mash session started on %s", username, ctime(&now));
}

/*
 * Sets the current work-directory, starting with ~ for the home directory
 */
void setCwd(){
	if (getcwd(cwd, sizeof(cwd)) != NULL){
		char* lowerCaseUsername = malloc(strlen(username) + 1);
		strcpy(lowerCaseUsername, username);
		lowerCaseUsername[0] = tolower(lowerCaseUsername[0]);
		char *s;
 		s = strstr(cwd, lowerCaseUsername);

 		if (s != NULL){
 			//Remove every character before up until the home directory and replace it with a ~
 			memmove(cwd, cwd + (s - cwd + strlen(username)) - 1, strlen(cwd+1));
 			cwd[0] = '~';
 		}
	}else{
		errorLogger(1, "getcwd() error");
	}
}

/*
 * Split line into command and arguments, then check via stat() if the file exists.
 * If file exists or may be in path, execute it. Otherwise exit.
 */
int parseCmdLine(char* cmd){
	char *splitCmd, *savePtr;
	char *cmdArgs[MAX_ARGS];
	int i = 1;

	//Splits the string by space or tabs.
	splitCmd = strtok_r(cmd, "	 ", &savePtr);
	cmdArgs[0] = splitCmd;

	//Chec if user is calling a file to execute, rather than a 'command' in Path
	if(cmdArgs[0][0] == '/' || cmdArgs[0][0] == '.'){
		struct stat fileStat;
	    if(stat(cmdArgs[0],&fileStat) < 0){
	    	printf("The file " CCYAN "%s" CRESET " you were looking for %s, does unfortunately not exist there.\n", cmdArgs[0], username);    
	        _exit(0);	
	    }
	}

	//Split command/arguments as long as everything is split, or the limit is reached.
	while (splitCmd != NULL) {
    	splitCmd = strtok_r(NULL, "	 ", &savePtr);
    	if(splitCmd != NULL){
    		if(i < MAX_ARGS - 1){
    			cmdArgs[i] = splitCmd;
    			i++;
    		}else{
    			errorLogger(1, "Sorry, you can't enter more than 14 arguments. Executing may fail now. \n");
    			break;
    		}	
    	}
  	}
  	cmdArgs[i] = NULL;
  	//If command is succesfully executed, the program will never reach the printf or _exit();
  	execvp(cmdArgs[0], cmdArgs);
  	printf("%s, " CCYAN "%s" CRESET " is unfortunately not a command.\n", username, cmdArgs[0]);
  	_exit(0);		
}

/*
 * main function - executes ecerything and runs in infinite loop until stopped via CTRL+C
 */
int main(void){
	//Initialize current time for Session-start
	time(&now);
	//Get the username and capitalize it.
	username = getlogin();
  	username[0] = toupper(username[0]);

  	//prints a one-time welcome message
	promptWelcomeMsg();

	//Handle CTRL+C and CTRL+Z 'manually'.
	//The strategy basically is: Do nothing for the parent process when there is a child-process
	signal(SIGINT, interruptHandler);
	signal(SIGTSTP, tstpHandler);

	//initializes variables for reading line and prompt-message
	char *line;
	char mashPrompt[LINE_LENGTH + 3];

	//set 'cwd' to the current work directory and set prompt-message
	setCwd();
	snprintf(mashPrompt, sizeof(mashPrompt), "%s >> ", cwd);

	//reads lines until EOF is detected, using the GNU readline library
	while((line = readline(mashPrompt)) != NULL){
		int status;

        //If user presses enter without any character ignore the command
        if (line[0] != 0){
        	//saving commant to history, using GNU readline library
        	add_history(line);
        }else{
        	continue;
        }

		//Forking the child process, which then handles the read line, as well as error handling.
		if ((childPID=fork())==0){
			//For child-process only; parses and executes command
			//printf("\nChild Process with %u started. Parent Process ID: %u \n", getpid(), getppid());
    		parseCmdLine(line);
		}else if(childPID == -1){
			//Failure, no child process is created
			fprintf(stderr, "Unfortunately there was a little mess-up, %s\n", username);
		}else{
			//For parent-process: Waits on child
			pid_t result = waitpid(childPID, &status, WUNTRACED);
			if (result == -1) {
				//waitpid was not succesfull
				fprintf(stderr, "Something under the hood went terribly wrong. Just try again, %s", username);
			} else {
				//Everything succesfully executed; Set childPID to -1, 
				//so signal-handler functions know there is no child-process running
				childPID = -1;
				free(line);
			}
		}
		//Theoretically to check if the user has changed directory. Since cd isn't built in yet, doesn't really have a function
		setCwd();
		snprintf(mashPrompt, sizeof(mashPrompt), "%s >> ", cwd);
	}
	return EXIT_SUCCESS;
}
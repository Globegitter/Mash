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
 * Should print current work-directory
 */
void promptCmdMsg(){
	char cwdTemp[LINE_LENGTH];
	const char *homeDir = getenv("HOME");
	//printf("GetHomeEnv: %s", homeDir);
	//printf("Length %d", strlen(homeDir));
  	//printf("ROOT : %s\n", getenv("ROOT"));
  	//printf("HOSTNAME : %s\n", getenv("HOSTNAME"));
  	//printf("CDPATH : %s\n", getenv("CDPATH"));
	if (getcwd(cwd, sizeof(cwd)) != NULL && homeDir){
		char* lowerCaseUsername = username;
		lowerCaseUsername[0] = tolower(lowerCaseUsername[0]);
		char *s;

 		s = strstr(cwd, lowerCaseUsername);
 		if (s != NULL){
 			printf("Found string at index = %d\n", s - cwd + strlen(username));
 			printf("Length of found string: %i", strlen(s));
 			printf("%s", s);
 		}else{
 			printf("String not found\n");  // `strstr` returns NULL if search string not found
 		}


		//Seems on Linux there is a small error here.
		strncpy(cwdTemp, cwd, strlen(cwd));
		//memmove(cwdTemp, cwdTemp + strlen(homeDir), strlen(cwdTemp+1));
		printf("\n~%s >> ", cwdTemp);
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
    	cmdArgs[i] = splitCmd;
    	i++;
    	if(i == MAX_ARGS){
    		errorLogger(1, "Sorry, you can't enter more than 15 arguments. Will try to execute that.");
    		break;
    	}
  	}
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

	ssize_t bytesRead = 0;

	while(bytesRead != -1){
		int status;
		size_t len = LINE_LENGTH;

		char *line;
  		line = (char *) malloc (LINE_LENGTH + 1);

  		//prompts the current work-directory before every command
		promptCmdMsg();

		//Read the current line entered by the user or inputfile
		if((bytesRead = getline(&line, &len, stdin)) == -1){
			errorLogger(1, "Could not read command or reached end of file.\n");
			return EXIT_SUCCESS;	
		}

		//technically not necessary, but set the last character to end of string.
		line[strlen(line) - 1] = '\0';

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
			}
		}
	}
	return EXIT_SUCCESS;
}
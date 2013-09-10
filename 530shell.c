/*
 * Author: Markus Padourek]
 * Date: 08/09/2013 (dd/mm/yy)
 * Mash - v0.0.0
 * Shell for Unix systems
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

time_t now, end;

void errorLogger(int errLevel, char* err){
	fprintf(stderr, "%s", err);
}

void promptWelcomeMsg(){
	printf("Mash session started at %s", ctime(&now));
}

void promptCmdMsg(){
	char cwd[1024];
	const char *homeDir = getenv("HOME");
	if (getcwd(cwd, sizeof(cwd)) != NULL && homeDir){
		memmove(cwd, cwd + strlen(homeDir), strlen(cwd+1));
		printf("\n~%s >> ", cwd);
	}else{
		errorLogger(1, "getcwd() error");
	}
}

void parseCmdLine(char* cmd){
	char *splitCmd, *savePtr;
	//improve by making this array 'dynamic' in size
	char *cmdArgs[10];
	//char* argv[] = {"say", "'Hello'", NULL};
	int i = 1;

	splitCmd = strtok_r(cmd, "	 ", &savePtr);
	cmdArgs[0] = splitCmd;
	while (splitCmd != NULL) {
    //	printf ("%s\n", splitCmd);
    	splitCmd = strtok_r(NULL, "	 ", &savePtr);
    	//if(&splitCmd != '\n'){
    		cmdArgs[i] = splitCmd;
    	//}
    	//printf ("%s\n", cmdArgs[i]);
    	i++;
  	}
  	execvp(cmdArgs[0], cmdArgs);
  	_exit(1);		
}

int main(void){
	//Initialize current time for Session-start
	time(&now);
	promptWelcomeMsg();
	while(1){
		int childPID, status;

		char *line;
		size_t len = 1024;
  		ssize_t bytesRead;
  		line = (char *) malloc (len + 1);

		promptCmdMsg();

		if((bytesRead = getline(&line, &len, stdin)) == -1){
			errorLogger(1, "Couldn't read command");	
		}
		line[strlen(line) - 1] = '\0';
		if ((childPID=fork())==0){
    		printf("\nChild Process with %u started. Parent Process ID: %u \n", getpid(), getppid());
    		parseCmdLine(line);
		}else{ /* avoids error checking*/
    		//printf("Dont yada yada me, im your parent with pid %u ", getpid());
    		//int status;
    		//You can also try WNOHANG instead of 0
			pid_t result = waitpid(childPID, &status, 0);
			if (result == 0) {
			  // Child still alive
				printf("Child still alive. Parent: %u \n", getpid());
			} else if (result == -1) {
			  // Error 
			} else {
			  printf("Ended child process %i \n", childPID);
			}
    		//waitpid(childPid, &status, 0);
		}
	}
	return EXIT_SUCCESS;
}
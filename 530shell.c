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
		printf("~%s >> ", cwd);
	}else{
		errorLogger(1, "getcwd() error");
	}
}

void parseCmdLine(){

    

	
}

int main(void){
	//Initialize current time for Session-start
	time(&now);
	promptWelcomeMsg();
	while(1){
		int childPid

		char *cmd;
		size_t len = 1024;
  		ssize_t bytesRead;
  		cmd = (char *) malloc (len + 1);

		promptCmdMsg();

		if((bytesRead = getline(&cmd, &len, stdin)) == -1){
			errorLogger(1, "Couldn't read command");	
		}
	}
	return EXIT_SUCCESS;
}
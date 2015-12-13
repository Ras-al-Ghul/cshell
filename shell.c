#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>

#define MAXNUM 100
#define MAXCMD 100
#define MAXOP 100

//Variables to ensure that the shell is initialized properly
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;
pid_t fpid = 0;	//Holds foreground process pid
char fname[MAXNUM];	//Holds foreground process
char infile[MAXNUM],outfile[MAXNUM];

//For holding name of background processes
struct bground{
	int pid;
	char name[MAXNUM];
};

struct bground barray[MAXNUM * MAXOP];
int barraycnt = 0;
//Done

//For holding background jobs
struct alljobs{
	int count;
	int state;
	int pid;
	int stopped;
	char name[MAXNUM];
};
struct alljobs jobsarray[MAXNUM * MAXOP];
int jobsarraycnt = 5;
//Done

//For holding status of exited background process
struct bgroundexit{
	char name[MAXNUM];
	int pid;
	int status;
	int used;
};

struct bgroundexit bgrounds[MAXNUM * MAXOP];
//Done


void signal_handler (int signum){//Handles exit message of background processes
  	int pid, status,i;
  	while (1){
      		pid = waitpid (WAIT_ANY, &status, WNOHANG);
      		if (pid < 0){
          		//perror ("waitpid");
          		break;
        	}
      		if (pid == 0)
        		break;
			int tmpcount = 0;

			while(barray[tmpcount].pid != pid && tmpcount<(MAXNUM*MAXOP))
				tmpcount++;
			if(( tmpcount == (MAXNUM * MAXOP) ) || barray[tmpcount].pid != pid)
				return;

			for(i=0 ; i<(MAXNUM * MAXOP) ; i++){
				if(jobsarray[i].pid == pid){
					jobsarray[i].state = 0;
					break;
				}
			}

			if(status == 0){
				i=0;
				while(bgrounds[i].used != 0 && i < (MAXNUM*MAXOP))
					i++;
				if(i == (MAXNUM * MAXOP))
					return;
				bgrounds[i].used = 1;
				bgrounds[i].status = 1;
				bgrounds[i].pid = pid;
				strcpy(bgrounds[i].name,barray[tmpcount].name);

			}
			else{
				i=0;
				while(bgrounds[i].used != 0 && i < (MAXNUM*MAXOP))
					i++;
				if(i == (MAXNUM * MAXOP))
					return;
				bgrounds[i].used = 1;
				bgrounds[i].status = status;
				bgrounds[i].pid = pid;
				strcpy(bgrounds[i].name,barray[tmpcount].name);
			}

			if(pid!=-1&&pid!=0)
			{
				if(WIFEXITED(status))				
				{
					int i;
					for(i=0;i<(MAXNUM*MAXOP);i++){
						if(jobsarray[i].pid == pid){
								fprintf(stdout,"%s with pid %d exited normally\n",jobsarray[i].name,jobsarray[i].pid);
								jobsarray[i].state = 0;
							}
					}
				}
				else if(WIFSIGNALED(status))
				{
					int i;
					for(i=0;i<(MAXNUM*MAXOP);i++){
						if(jobsarray[i].pid == pid){
								fprintf(stdout,"%s with pid %d signalled to exit\n",jobsarray[i].name,jobsarray[i].pid);
								jobsarray[i].state = 0;
							}
					}
				}
				else if(WIFEXITED(status))
				{
					int i;
					for(i=0;i<(MAXNUM*MAXOP);i++){
						if(jobsarray[i].pid == pid){
								fprintf(stdout,"%s with pid %d exited normally\n",jobsarray[i].name,jobsarray[i].pid);
								jobsarray[i].state = 0;
							}
					}
				}
			}


    }
	return;
}

void init_shell (){

  	/* See if we are running interactively.  */
  	shell_terminal = STDIN_FILENO;
  	shell_is_interactive = isatty (shell_terminal);

	if (shell_is_interactive){
      		/* Loop until we are in the foreground.  */
      		while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
        		kill (- shell_pgid, SIGTTIN);

      		/* Ignore interactive and job-control signals.  */
		    signal (SIGINT, SIG_IGN);
			signal (SIGTSTP, SIG_IGN);
			signal (SIGQUIT, SIG_IGN);
			signal (SIGTTIN, SIG_IGN);
			signal (SIGTTOU, SIG_IGN);

      		/* Put ourselves in our own process group.  */
      		shell_pgid = getpid ();
      		if (setpgid (shell_pgid, shell_pgid) < 0){
          		perror ("Couldn't put the shell in its own process group");
          		exit (1);
        	}

      		/* Grab control of the terminal.  */
      		tcsetpgrp (shell_terminal, shell_pgid);

      		/* Save default terminal attributes for shell.  */
      		tcgetattr (shell_terminal, &shell_tmodes);
    	}
}

void readLine(char *wholecommand){
	char *line = NULL;
	size_t len = 0;

	getline(&line, &len, stdin);

	strcpy(wholecommand,line);
	return;
}

void initarray(char *arrs[MAXCMD][MAXOP]){
	//Initialize command 2D array
	int j,k;
	for(j=0;j<MAXCMD;j++)
		for(k=0;k<MAXOP;k++)
			arrs[j][k] = NULL;
	//Initialize done
	return;
}

void tokenize(char *str1,char *arr[MAXNUM],char *token,char *dupdir){
	//Tokenize directory path
	int i,k;
	for(i=0;i<MAXNUM;i++)
		arr[i] = NULL;
	for(str1=dupdir,k=0;;str1=NULL){
		token = strtok(str1,"/");
		if(token == NULL)
			break;
		arr[k++]=token;
	}
	return;
}

void exececho(char *wholecommand,char *cmdarr[MAXCMD][MAXOP],int i){
	char tempwholecmd[MAXNUM];tempwholecmd[0] = '\0';
	strcpy(tempwholecmd,wholecommand);
	int k = 1;
	while(cmdarr[i][k]!=NULL)
		printf("%s ",cmdarr[i][k++]);
	printf("\n");
	return;
}

void execpwd(){
	char temppwd[MAXNUM];
	getcwd(temppwd,MAXNUM);
	printf("%s\n",temppwd);
	return;
}

void execcd(char *cmdarr[MAXCMD][MAXOP],char *homedir,int i){
	int k;
	char temppath[MAXNUM];temppath[0] = '\0';
	if(cmdarr[i][1] == NULL){
		strcpy(temppath,homedir);
		chdir(temppath);
		return;
	}
	strcpy(temppath,cmdarr[i][1]);
	char temphomepath[MAXNUM]; temphomepath[0] = '\0'; strcat(temphomepath,homedir);
	if(temppath[0] == '~'){
		k=0;
		while(temppath[k+1]!='\0'){
			temppath[k]=temppath[k+1];
			k++;
		}
		temppath[k] = '\0';
		strcat(temphomepath,temppath);
		strcpy(temppath,temphomepath);
	}
	int retval = chdir(temppath);
	if(retval != 0){
		printf("cd Unsuccesful. No such directory found\n");
	}
	return;
}

void pipetokenize(char *cmdarr[MAXCMD][MAXOP],char *pipecmdarr[MAXCMD][MAXCMD][MAXOP],int *numofpipes){		//Tokenize cmdarr further if there are pipes in the command
	int pipecount;	
	int i,j,k,start,pipenum;
    	for(i=0;i<MAXCMD;i++){
        	start = 0;pipenum = 0;pipecount = 0;
        	for(j=0;j<MAXOP;j++){
				if(cmdarr[i][0] == NULL){				
					break;
				}
				if(cmdarr[i][j] == NULL){
					for(k=start;k<j;k++){
						char * temp = (char *) malloc(sizeof(char)*MAXNUM);
						strcpy(temp,cmdarr[i][k]);
                    	(pipecmdarr[i][pipenum][k-start] = temp);
                	}
					break;
				}
            	if(strcmp(cmdarr[i][j],"|")==0){
                	for(k=start;k<j;k++){
                    	char * temp = (char *) malloc(sizeof(char)*MAXNUM);
						strcpy(temp,cmdarr[i][k]);
                    	(pipecmdarr[i][pipenum][k-start] = temp);
                	}
					pipecount++;
                	start = j+1;
                	pipenum++;
            	}
        	}
			numofpipes[i] = pipenum;
    	}
	
	return;
}

void pipetokenizetest(char *cmdarr[MAXCMD][MAXOP],char *pipecmdarr[MAXCMD][MAXCMD][MAXOP],int *numofpipes){
    	int i,j,k;
    	for(i=0;i<MAXCMD;i++){
        	for(j=0;j<MAXCMD;j++){
            		for(k=0;k<MAXOP;k++){
                		if(pipecmdarr[i][j][k]==NULL)
                    			break;
                		printf("%d%d%d %s ",i,j,k,pipecmdarr[i][j][k]);
            		}
            		if(pipecmdarr[i][j][0]==NULL)
                		break;
            		printf("\t\t");
        	}
        	if(pipecmdarr[i][0][0]==NULL)
                	continue;
			printf("%d\n",numofpipes[i]);
        	printf("\n");
    	}
	
    	return;
}

void tokentest(char *cmdarr[MAXCMD][MAXOP]){
	int j,k;
	//Tokenizing test begins
	for(j=0;j<MAXCMD;j++){
		for(k=0;k<MAXOP;k++){
			if(cmdarr[j][k] == NULL)
				break;
			printf("%d%d \t %s \t",j,k,cmdarr[j][k]);
		}
		if(cmdarr[j][0] == NULL)
				continue;
		printf("\n");
	}
	//Tokenizing test ends
	printf("\n");
	return;
}

void maintokenize(char *cmdarr[MAXCMD][MAXOP],char *dupwholecommand){
	int j,k;
	char *str1 = NULL,*str2 = NULL;
	char *saveptr1 = NULL,*saveptr2 = NULL;
	char *token = NULL,*subtoken = NULL;
	//Tokenizing begins
	for (j = 0, str1 = dupwholecommand; ; j++, str1 = NULL) {
		token = strtok_r(str1, ";", &saveptr1);
		if (token == NULL)
                   	break;
		for (k = 0, str2 = token; ; str2 = NULL,k++) {
	                   subtoken = strtok_r(str2, " \t", &saveptr2);
	                   if (subtoken == NULL)
	                       	break;
			cmdarr[j][k] = subtoken;
		}
	}
	//Tokenizing ends
	return;
}

void initpipecmd(char *pipecmdarr[MAXCMD][MAXCMD][MAXOP]){   	
	//Initialize pipe 3D array
	int i,j,k;
	for(i=0;i<MAXCMD;i++)
        	for(j=0;j<MAXCMD;j++)
            		for(k=0;k<MAXOP;k++)
                		pipecmdarr[i][j][k] = NULL;
	//Initialize done
	return;
}

void directorysettings(char *cwdir,char *homedir,char *dupcwdir,char *hdir[MAXNUM]){
	char *str1 = NULL,*token = NULL;
	//Directory settings for prompt
	if(strcmp(cwdir,homedir) != 0){
		//Tokenize current directory
		char *tmp[MAXNUM];
		tokenize(str1,tmp,token,dupcwdir);

		int j=0,k=0;
		char tempstr1[MAXNUM] ,tempstr2[MAXNUM] ;
		tempstr1[0] = '\0';tempstr2[0] = '\0';
		strcat(tempstr1,tmp[k++]);strcat(tempstr2,hdir[j++]);

		if(strcmp(tempstr1,tempstr2) == 0 && strlen(cwdir) > strlen(homedir)){
			while(strcmp(tempstr1,tempstr2) == 0){
				tempstr1[0]='\0';	tempstr2[0]='\0';
				if(hdir[j]!=NULL){
					strcat(tempstr1,tmp[k++]);	strcat(tempstr2,hdir[j++]);
				}
				else
					break;
			}
			cwdir[0] = '~',cwdir[1] = '\0';

			while(tmp[k++] != NULL){
				strcat(cwdir,"/");
				strcat(cwdir,tmp[k-1]);
			}


		}
		else{
			getcwd(cwdir,MAXNUM);
		}
	}
	else
		cwdir[0] = '~',cwdir[1] = '\0';

	//Directory settings for prompt done

	return;
}

int checkinfile(char *cmdarr[MAXCMD][MAXOP],int i){		// To check if there is input redirection
	int j=1;
	while(cmdarr[i][j] != NULL){
		if(strcmp(cmdarr[i][j],"<") == 0){
			if(fopen(cmdarr[i][j+1],"r") == NULL){
				printf("No such file exists\n");
				return 0;
			}
			strcpy(infile,cmdarr[i][j+1]);
			int k;
			for(k=j; k<(MAXOP-2); k++){
				cmdarr[i][k] = cmdarr[i][k+2];
			}
			cmdarr[i][MAXOP-1] = NULL;
			return 1;
		}
		j++;
	}
	strcpy(infile,"\0");
	return 0;
}

int checkoutfile(char *cmdarr[MAXCMD][MAXOP],int i){	//To check if there is output redirection
	int j=1;
	while(cmdarr[i][j] != NULL){
		if(strcmp(cmdarr[i][j],">") == 0){
			if(cmdarr[i][j+1] != NULL){			
				strcpy(outfile,cmdarr[i][j+1]);
				int k;
				for(k=j; k<(MAXOP-2); k++){
					cmdarr[i][k] = cmdarr[i][k+2];
				}
				cmdarr[i][MAXOP-1] = NULL;
				return 1;
			}
			else{
				printf("Unexpected error near > token\n");
				strcpy(outfile,"\0");
				return 0;
			}
			
		}
		if(strcmp(cmdarr[i][j],">>") == 0){
			if(cmdarr[i][j+1] != NULL){			
				strcpy(outfile,cmdarr[i][j+1]);
				int k;
				for(k=j; k<(MAXOP-2); k++){
					cmdarr[i][k] = cmdarr[i][k+2];
				}
				cmdarr[i][MAXOP-1] = NULL;
				return 2;
			}
			else{
				printf("Unexpected error near > token\n");
				strcpy(outfile,"\0");
				return 0;
			}
			
		}
		
		j++;
	}
	strcpy(outfile,"\0");
	return 0;
}


int executeCommand(char *cmdarr[MAXCMD][MAXOP],int i,int flag, pid_t shellpid){
	if (shell_is_interactive){


		if(checkinfile(cmdarr,i) == 1){		//For input redirection
				
			if(strcmp(infile,"\0") != 0){
				int in=open(infile,O_RDONLY);
				dup2(in,0);
				close(in);
			}
		}

		int outretval = checkoutfile(cmdarr,i);
		if(outretval == 1 || outretval == 2){		//For output redirection
			
			if(strcmp(outfile,"\0") != 0){
				if(outretval == 1){
					int out=open(outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
					dup2(out,1);
					close(out);
				}
				else{
					if(fopen(outfile,"r") == NULL){
						int out=open(outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						dup2(out,1);
						close(out);
					}
					else{
						int out=open(outfile,O_WRONLY | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						dup2(out,1);
						close(out);
					}
				}
			}
		}

      		setpgid(getpid(),getpid());		//Create process group
      		if(!flag)
				tcsetpgrp(shell_terminal,getpid());		//If foreground process, give it terminal control

			//Signals
			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);
			signal (SIGTSTP, SIG_DFL);
			signal (SIGTTIN, SIG_DFL);
			signal (SIGTTOU, SIG_DFL);
			signal (SIGCHLD, SIG_DFL);
	}


	return execvp(cmdarr[i][0],cmdarr[i]);
}

void execoverkill(){	//Kills all background processes
	int i;
	for(i = 0; i<(MAXNUM*MAXOP) ; i++){
		if(jobsarray[i].state == 1){
			jobsarray[i].state = 0;
			kill(jobsarray[i].pid,SIGKILL);
			if(jobsarray[i].stopped == 1){
				printf("%s with pid %d killed\n",jobsarray[i].name,jobsarray[i].pid);
			}
		}
	}
	return;
}

void execjobs(){		//Shows an ordered list of current background processes in their order of creation
	int i;
	for(i=0 ; i<(MAXNUM*MAXOP) ; i++){
		if(jobsarray[i].state == 1){
			printf("[%d] %s [%d]\n",jobsarray[i].count,jobsarray[i].name,jobsarray[i].pid);
		}
	}
	return;
}

void execfg(char *cmdarr[MAXCMD][MAXOP],int i){		//Brings the specified background process to the foreground
	if(cmdarr[i][1] == NULL){
		printf("Takes one argument\n");
		return;
	}
	strcpy(fname,cmdarr[i][0]);
	int j,temppid = 0;
	
	int tempposn = atoi(cmdarr[i][1]);
	//int tempcnt = 0;
	for(j=0;j<(MAXNUM*MAXOP);j++){
		if(jobsarray[j].count == tempposn && jobsarray[j].state == 1){
			temppid = jobsarray[j].pid;		
			break;
		}
	}
	
	if(temppid > 0){
		for(j=0;j<(MAXNUM*MAXOP);j++){
			if(jobsarray[j].pid == temppid && jobsarray[j].state == 1){
				strcpy(fname,jobsarray[j].name);
				jobsarray[j].state = 0;
				fpid = temppid;



				int temppgid=getpgid(temppid);
				
				tcsetpgrp(shell_terminal,temppgid);

				killpg(temppgid,SIGCONT);

				int status;
				do{
					int w = waitpid(temppid, &status, WUNTRACED | WCONTINUED);
            				if (w == -1) {

						perror("waitpid"); exit(EXIT_FAILURE);
					}

					if (WIFEXITED(status)) {
            					printf("exited, status=%d\n", WEXITSTATUS(status));
						break;
					}
					else if (WIFSIGNALED(status)) {
                				printf("killed by signal %d\n", WTERMSIG(status));
						break;
            				}
					else if (WIFSTOPPED(status)) {
        				printf("\nstopped by signal %d\n", WSTOPSIG(status));
        				if(WSTOPSIG(status) == 19 || WSTOPSIG(status) == 20){
									strcpy(jobsarray[jobsarraycnt].name,fname);
									jobsarray[jobsarraycnt].pid = fpid;
									jobsarray[jobsarraycnt].stopped = 1;
									jobsarray[jobsarraycnt++].state = 1;
								}
        						printf("\nstopped by signal %d\n", WSTOPSIG(status));
        						if(WSTOPSIG(status) == 22)
        							kill(fpid,SIGKILL);
						break;
            		}
					else if (WIFCONTINUED(status)) {
                				printf("continued\n");
            				}
				}while(!WIFEXITED(status) && !WIFSIGNALED(status));
				tcsetpgrp (shell_terminal, shell_pgid);

				return;
			}
		}
	}
	printf("No background job with given pid exists\n");
	return;
}

void execkjob(char *cmdarr[MAXCMD][MAXOP],int i){		//Sends a specified signal to the specified process
	if(cmdarr[i][1] == NULL || cmdarr[i][2] == NULL){
		printf("Takes two arguments\n");
		return;
	}

	int temppid = 0,j;
	int tempposn = atoi(cmdarr[i][1]);
	//int tempcnt = 0;
	for(j=0;j<(MAXNUM*MAXOP);j++){
		if(jobsarray[j].count == tempposn && jobsarray[j].state == 1){
			temppid = jobsarray[j].pid;		
			break;
		}
	}

	int tempsig = atoi(cmdarr[i][2]);
	if(tempsig == 18 || tempsig == 9){
		if(tempsig == 18){
			execfg(cmdarr,i);
			return;
		}
		if(tempsig == 9){
			int i;
			for(i = 0 ; i < (MAXNUM*MAXOP) ; i++){
				if(jobsarray[i].pid == temppid){
					jobsarray[i].state = 0;
					if(jobsarray[i].stopped == 1)
						printf("%s with pid %d killed\n",jobsarray[i].name,jobsarray[i].pid);
			
					break;
				}
			}
		}
	}
	kill(temppid,tempsig);
	return;
}

void rectifyjobsarray(){		//Sends 0 signal to processes in jobsarray to check if they are still alive
	int i,retval,tempcount = 1;
	for(i=0 ; i<(MAXNUM*MAXOP) ; i++){
		retval = kill(jobsarray[i].pid,0);
		if(retval != 0 && errno == ESRCH){
			jobsarray[i].state = 0;
			continue;
		}
		if(retval == 0 && jobsarray[i].state == 1){			
			jobsarray[i].count = tempcount++;
		}
	}
	return;
}

void init_jobsarray(){
	int i;
	for(i=0 ; i<(MAXNUM*MAXOP) ; i++){
		jobsarray[i].state = 0;
	}
	return;
}

void getpinfo(pid_t pid, int flag,char *homedir){		//Gets process info

	unsigned long memory;
	char status;
	char procname[MAXNUM];

	char name[MAXNUM];
	char buf[MAXNUM*MAXNUM];
    	FILE *proc;
	sprintf(name,"/proc/%d/status",pid);
    	proc = fopen(name,"r");

	if(proc){
		fgets(buf,((MAXNUM*MAXNUM)),proc);sscanf(buf,"Name:\t%s",procname);
		fseek(proc, 0 , SEEK_SET);
		int tempcnt = 1;
		while(tempcnt < 3){
			fgets(buf,((MAXNUM*MAXNUM)),proc);
			tempcnt++;
		}
		sscanf(buf,"State:\t%c",&status);
		fseek(proc, 0 , SEEK_SET);
		tempcnt = 1;
		while(tempcnt < 14){
			fgets(buf,((MAXNUM*MAXNUM)),proc);
			tempcnt++;
		}
		sscanf(buf,"VmSize:\t%lu",&memory);
	}
	else
		return;
	printf("Name %s\n",procname);
	printf("PID %d\n",pid);
	printf("Process status %c\n",status);
	printf("Virtual memory %lu kB\n",memory);
	fclose(proc);
	sprintf(name,"/proc/%d/exe",pid);
	proc = fopen(name,"r");
	if(flag == 0){
		char execPath[MAXNUM];
		execPath[0] = '\0';
		strcpy(execPath,homedir);strcat(execPath,"/a.out");
		printf("Executable path  %s\n",execPath);
	}
	else{
		if(proc){
			char *execPath =(char *) malloc(sizeof(char)*MAXNUM);
			execPath[0] = '\0';
			readlink(name,execPath,MAXNUM);
			printf("Executable path %s\n",execPath);
			free(execPath);
		}
		else
			return;
	}
	fclose(proc);
	return;
}

int pipesexecute(char *cmdarr[MAXCMD][MAXOP],char *pipecmdarr[MAXCMD][MAXCMD][MAXOP],int *numofpipes,int ii){		//Execute piped commands

	int dupstdin = dup(0);
	int dupstdout = dup(1);

	

	int i,j=numofpipes[ii],k,pipes[2*numofpipes[ii]],pgid,comc = 0;

	for(i=0;j--;i+=2){
		if((pipe(pipes+i))<0)						//open the pipes
		{	
			perror("pipe error");
			return -1;
		}
	}

	j=0;k=0;
	while(j < (numofpipes[ii]+1)){
		
		strcpy(infile,"\0");strcpy(outfile,"\0");
		
		k = 0;
		while(pipecmdarr[ii][j][k] != NULL){
			if(strcmp(pipecmdarr[ii][j][k],"<") == 0){
				if(fopen(pipecmdarr[ii][j][k+1],"r") == NULL){
					printf("No such file exists\n");
					return -1;
				}
				strcpy(infile,pipecmdarr[ii][j][k+1]);
				int kk;
				for(kk=k; kk<(MAXOP-2); kk++){
					pipecmdarr[ii][j][kk] = pipecmdarr[ii][j][kk+2];
				}
				break;
			}
			k++;
		}


		

		k=0;
		int outretval = 0;
		while(pipecmdarr[ii][j][k] != NULL){
			
			if(strcmp(pipecmdarr[ii][j][k],">") == 0){
				if(pipecmdarr[ii][j][k+1] != NULL){			
					strcpy(outfile,pipecmdarr[ii][j][k+1]);
					outretval = 1;
					int kk;
					for(kk=k; kk<(MAXOP-2); kk++){
						pipecmdarr[ii][j][kk] = pipecmdarr[ii][j][kk+2];
					}
					break;
				}
				else{
					printf("Unexpected error near > token\n");
					strcpy(outfile,"\0");
					return -1;
				}
				
			}
			if(strcmp(pipecmdarr[ii][j][k],">>") == 0){
				if(pipecmdarr[ii][j][k+1] != NULL){			
					strcpy(outfile,pipecmdarr[ii][j][k+1]);
					outretval = 0;
					int kk;
					for(kk=k; kk<(MAXOP-2); kk++){
						pipecmdarr[ii][j][kk] = pipecmdarr[ii][j][kk+2];
					}
					break;
				}
				else{
					printf("Unexpected error near > token\n");
					strcpy(outfile,"\0");
					return -1;
				}
				
			}
			k++;
		}

		int pid = fork(),in,out;

		if(pid > 0){//Parent
			if(comc == 0)
				pgid=pid;							
			if(pid != 0)			
				setpgid(pid,pgid);
		}
		else if(pid==0){
			/*set the signal of child to default*/
			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);
			signal (SIGTSTP, SIG_DFL);
			signal (SIGTTIN, SIG_DFL);
			signal (SIGTTOU, SIG_DFL);
			signal (SIGCHLD, SIG_DFL);

			if(strcmp(outfile,"\0") != 0 && pipecmdarr[ii][j+1][0] == NULL){					//if there is an outfile	
				if(outretval == 1){
					out=open(outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
					dup2(out,1);
					close(out);
				}
				else{
					if(fopen(outfile,"r") == NULL){
						out=open(outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						dup2(out,1);
						close(out);
					}
					else{
						out=open(outfile,O_WRONLY | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						dup2(out,1);
						close(out);
					}
				}
				
			}
			else if(pipecmdarr[ii][j+1][0] != NULL){				//if not the last command
				if((dup2(pipes[2*comc+1],1))<0){
					perror("dup2 error");
				}
			}
			else{
				if((dup2(dupstdout,1))<0){
					perror("dup2 error");
				}
			}

			if(strcmp(infile,"\0") != 0 && comc == 0){					//if there is an infile
				in=open(infile,O_RDONLY);
				dup2(in,0);
				close(in);
			}
			else if(comc!=0){					//if not the first command
				if((dup2(pipes[2*(comc-1)],0))<0){
					perror("dup2 error");
				}
			}
			else{
				if((dup2(dupstdin,0))<0){
					perror("dup2 error");
				}
			}

			//close all pipes in the children
			for(i=0;i<2*(numofpipes[ii]);i++)				//close all pipes in the children
				close(pipes[i]);

			if((execvp(pipecmdarr[ii][j][0],pipecmdarr[ii][j]))<0){
				perror("Cannot execute");
				_exit(-1);
			}



		}
		else if(pid<0){
			perror("Could not fork child");
			return -1;
		}
		if(pipecmdarr[ii][j][0] != NULL)
			comc++;

		j++;
	}

	int status;									
	for(i=0;i<2*(numofpipes[ii]);i++)
	{
		close(pipes[i]);							//close all pipes in parent
	}
	
	
	tcsetpgrp(shell_terminal,pgid);						//set the terminal control to child
	for(i=0;i<(numofpipes[ii]+1);i++){
		waitpid(-pgid,&status,WUNTRACED);			//Wait for all processes in the pipe
		if(WIFSTOPPED(status))
			killpg(pgid,SIGSTOP);
	}
	tcsetpgrp(shell_terminal,shell_pgid);					//return control to parent
	

	return 0;
}

int main(int argc, char *argv[]){
	//Initialize the shell
	init_shell();

	int numofpipes[MAXCMD];

	//Initialize jobs array
	init_jobsarray();

	//Holds the command
	char wholecommand[MAXNUM];char dupwholecommand[MAXNUM];
	char *cmdarr[MAXCMD][MAXOP];	//The rows hold commands separated by ;
					//while the columns hold the command and its options separated by " " or \t
    char *pipecmdarr[MAXCMD][MAXCMD][MAXOP];

	char maincmd[MAXNUM];

	//For prompt
	char username[MAXNUM],sysname[MAXNUM],cwdir[MAXNUM];
	char homedir[MAXNUM];getcwd(homedir,MAXNUM);
	char duphomedir[MAXNUM]; duphomedir[0] = '\0'; strcat(duphomedir,homedir);

	//For tokenizing
	char *str1 = NULL;
	char *token = NULL;

	//Counters
	int i;

	//Initializing background process array
	for(i=0;i<(MAXNUM*MAXOP);i++){
		bgrounds[i].used = 0;
	}
	//Initializing ends

	//Initialize to NULL
	initarray(cmdarr);

	char *hdir[MAXNUM];
	//Tokenize home directory
	tokenize(str1,hdir,token,duphomedir);
	//Tokenize home directory done

	while(1){
		
		if(signal(SIGINT,signal_handler)==SIG_ERR)
			perror("Signal not caught!!");
		if(signal(SIGCHLD,signal_handler)==SIG_ERR)
			perror("Signal not caught!!");

		for(i=0;i<MAXCMD;i++)
			numofpipes[i] = 0;

		rectifyjobsarray();

		//Printing message on background process completion
		for(i=0;i<(MAXNUM*MAXOP);i++){
			if(bgrounds[i].used == 0)
				continue;
			bgrounds[i].used = 0;
			if(bgrounds[i].status == 1){
				//printf("Process %s with pid %d terminated normally\n",bgrounds[i].name,bgrounds[i].pid);
			}
			else{
				//printf("Process %s with pid %d terminated with status %d\n",bgrounds[i].name,bgrounds[i].pid,bgrounds[i].status);
			}
		}

		//Renitializing background process array
		for(i=0;i<(MAXNUM*MAXOP);i++){
			bgrounds[i].used = 0;
		}
		//Renitializing ends

		//Initialize to NULL
		initarray(cmdarr);
        initpipecmd(pipecmdarr);

		//For prompt
		username[0] = '\0';cwdir[0] = '\0';
		strcat(username,getlogin());gethostname(sysname,MAXNUM);getcwd(cwdir,MAXNUM);
		char dupcwdir[MAXNUM];dupcwdir[0] = '\0'; strcat(dupcwdir,cwdir);

		directorysettings(cwdir,homedir,dupcwdir,hdir);
		//Prompt settings done

		//Take commands
		printf("<%s@%s:%s>",username,sysname,cwdir);
		wholecommand[0] = '\0';dupwholecommand[0] = '\0';
		readLine(wholecommand);
		wholecommand[strlen(wholecommand)-1]='\0';
		strcpy(dupwholecommand,wholecommand);
		//Done taking commands

		//Tokenize
		maintokenize(cmdarr,dupwholecommand);
		//tokentest(cmdarr);
		pipetokenize(cmdarr,pipecmdarr,numofpipes);
		//pipetokenizetest(cmdarr,pipecmdarr,numofpipes);
		//exit(EXIT_SUCCESS);
		//continue;
		//Tokenize done

		
		//Executing each ; separated command
		for(i = 0;i< MAXCMD;i++){
			if(cmdarr[i][0] == NULL)
				continue;
			maincmd[0] = '\0';
			strcat(maincmd,cmdarr[i][0]);
		
			strcpy(infile,"\0");
			strcpy(outfile,"\0");
		

			if(numofpipes[i] != 0){		//Execute piped commands
				pipesexecute(cmdarr, pipecmdarr, numofpipes, i);
				continue;
			}
	
			
			if((strcmp(maincmd,"echo") == 0) || (strcmp(maincmd,"pwd") == 0) || (strcmp(maincmd,"cd") == 0)\
			|| (strcmp(maincmd,"clear") == 0) || (strcmp(maincmd,"quit") == 0) || (strcmp(maincmd,"pinfo") == 0) ||
			strcmp(maincmd,"jobs") == 0 || strcmp(maincmd,"overkill") == 0 || strcmp(maincmd,"kjob") == 0
			|| strcmp(maincmd,"fg") == 0 ){

				int dupstdout = dup(1);
				int dupstdin = dup(0);	

				if(checkinfile(cmdarr,i) == 1){
				
					if(strcmp(infile,"\0") != 0){
						int in=open(infile,O_RDONLY);
						dup2(in,0);
						close(in);
					}
				}

				int outretval = checkoutfile(cmdarr,i);
				if(outretval == 1 || outretval == 2){
					
					if(strcmp(outfile,"\0") != 0){
						if(outretval == 1){
							int out=open(outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							dup2(out,1);
							close(out);
						}
						else{
							if(fopen(outfile,"r") == NULL){
								int out=open(outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
								dup2(out,1);
								close(out);
							}
							else{
								int out=open(outfile,O_WRONLY | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
								dup2(out,1);
								close(out);
							}
						}
					}
				}


				//echo execution
				if(strcmp (maincmd,"echo") == 0){
					exececho(wholecommand,cmdarr,i);
				}
				//echo done

				//pwd execution
				if(strcmp (maincmd,"pwd") == 0){
					execpwd();
				}
				//pwd done

				//chdir execution
				if(strcmp (maincmd,"cd") == 0){
					execcd(cmdarr,homedir,i);
				}
				//chdir done

				//clear execution
				if(strcmp (maincmd,"clear") == 0){
					printf("\e[1;1H\e[2J");
				}
				//clear done

				//terminal quit
				if(strcmp (maincmd,"quit") == 0){
					exit(EXIT_SUCCESS);
				}
				//terminal quit done

				//jobs
				if(strcmp (maincmd,"jobs") == 0){
					execjobs();
				}
				//jobs done

				//overkill
				if(strcmp (maincmd,"overkill") == 0){
					execoverkill();
				}
				//overkill done

				//kjob
				if(strcmp (maincmd,"kjob") == 0){
					execkjob(cmdarr,i);
				}
				//kjob done

				//fg
				if(strcmp (maincmd,"fg") == 0){
					execfg(cmdarr,i);
				}
				//fg done

				//pinfo
				if(strcmp (maincmd,"pinfo") == 0){
					pid_t proid;int flag = 0;
					if(cmdarr[i][1] == NULL)	//pinfo of shell
						proid = getpid();
					else{	//pinfo of given pid
						char temps[MAXNUM]; strcpy(temps,cmdarr[i][1]);
						proid = atoi(temps);
						if(kill(proid,0) == (-1)){
							printf("No such process exists\n");
							continue;
						}
						flag = 1;
					}
					getpinfo(proid, flag, homedir);
				}
				//pinfo done

				dup2(dupstdin,0);
				dup2(dupstdout,1);

			}
			else{
				pid_t shellpid = getpid();
				pid_t pid,w;
				int status;
				pid = fork();
				if(pid == 0 ){//Child Process
					int k = 0;int flag =0;
					while(cmdarr[i][k]!=NULL)
						k++;
					char tempandsym[2];tempandsym[0] = '\0';
					strcat(tempandsym,cmdarr[i][k-1]);
					if(strcmp(tempandsym,"&") == 0){
						cmdarr[i][k-1] = NULL;
						flag = 1;
					}

					status = executeCommand(cmdarr,i,flag,shellpid);
					if(status == -1){
						perror("No command found\n");
					}
					if(flag == 1){
						cmdarr[i][k-1] = "&";
					}
					tcsetpgrp (shell_terminal, shell_pgid);

  					tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes);
				}
				else if( pid > 0 ){//Parent process

					int k = 0;
					while(cmdarr[i][k]!=NULL)
						k++;
					char tempandsym[2];tempandsym[0] = '\0';
					strcat(tempandsym,cmdarr[i][k-1]);

					if(strcmp(tempandsym,"&")==0){//Store a background process's pid and name
						strcpy(barray[barraycnt].name, cmdarr[i][0]);
						barray[barraycnt++].pid = pid;


						//Store job details in jobsarray
						strcpy(jobsarray[jobsarraycnt].name,cmdarr[i][0]);
						jobsarray[jobsarraycnt].pid = pid;
						jobsarray[jobsarraycnt++].state = 1;
						//Storing done
					}
					else{//Wait for foreground process to complete
						

						fpid = pid;	//Store child pid of foreground process in global variable
						strcpy(fname,cmdarr[i][0]);	//Store name of foreground process

						tcsetpgrp(shell_terminal,fpid);

						do{
							w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
            						if (w == -1) {

								perror("waitpid"); exit(EXIT_FAILURE);
							}

							if (WIFEXITED(status)) {
            							 //printf("exited, status=%d\n", WEXITSTATUS(status));
								 break;
							}
							else if (WIFSIGNALED(status)) {
                						printf("killed by signal %d\n", WTERMSIG(status));
								break;
            						}
							else if (WIFSTOPPED(status)) {
										if(WSTOPSIG(status) == 19 || WSTOPSIG(status) == 20){
											strcpy(jobsarray[jobsarraycnt].name,fname);
											jobsarray[jobsarraycnt].pid = fpid;
											jobsarray[jobsarraycnt].stopped = 1;
											jobsarray[jobsarraycnt++].state = 1;
										}
                						printf("\nstopped by signal %d\n", WSTOPSIG(status));
                						if(WSTOPSIG(status) == 22)
                							kill(fpid,SIGKILL);
								break;
            						}
							else if (WIFCONTINUED(status)) {
                						printf("continued\n");
            						}			

						}while(!WIFEXITED(status) && !WIFSIGNALED(status));

						tcsetpgrp (shell_terminal, shell_pgid);
						fpid = shell_pgid;
  						tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes);
					}
				}
				else{//forking error
					fprintf(stderr, "fork failed\n");
					exit(EXIT_FAILURE);
				}
			}
					
		}
		//Execution done

	}//outer while loop ends

	exit(EXIT_SUCCESS);
}


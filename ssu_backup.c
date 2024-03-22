#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define LEN 1024

char *hashfunc = NULL;
char *backupDir = NULL;

int main(int argc, char *argv[])
{
	if(argc!=2){
		fprintf(stderr, "Usage: ssu_backup <md5 | sha1>\n");
		exit(0);
	}

	if(!(!strcmp(argv[1], "sha1") || !strcmp(argv[1], "md5"))){
		fprintf(stderr, "Usage: ssu_backup <md5 | sha1>\n");
		exit(0);
	}

	hashfunc = argv[1];

	// 백업 디렉터리 경로 저장
	backupDir = (char*)malloc(LEN*4);
	strcpy(backupDir, getenv("HOME"));
	strcat(backupDir, "/backup");

	// 백업 디렉터리 존재 확인. 없으면 생성
	if(access(backupDir, F_OK)==-1){
		if(mkdir(backupDir, 0777)==-1){
			fprintf(stderr, "make backup directory failed\n");
			exit(1);
		}
	}

	while(1){
		printf("20182654> ");

		// 명령어 입력 받기
		char command[LEN]; fgets(command, LEN, stdin);

		if(command[0]=='\n')
			continue;

		command[strlen(command)-1] = '\0';

		char *commands[LEN] = {NULL};
		int comidx = 0;

		// 명령어 토큰 분리
		commands[comidx++] = strtok(command, " ");
		while(commands[comidx] = strtok(NULL, " "))
			comidx++;

		// 명령어에 따라 자식 프로세스 생성 후 exec류 함수로 실행파일 실행
		if(!strcmp(commands[0], "add") || !strcmp(commands[0], "remove") || !strcmp(commands[0], "recover")){
			commands[comidx++] = hashfunc;
			pid_t pid = fork();
			if(pid==0){
				execv(commands[0], commands);
				exit(0);
			}
			else if(pid>0){
				wait(0);
			}
		}
		else if(!strcmp(commands[0], "ls")){
			commands[0] = (char*)malloc(LEN*4);
			strcpy(commands[0], "/bin/ls");

			pid_t pid = fork();
			if(pid==0){
				execv(commands[0], commands);
				exit(0);
			}
			else if(pid>0){
				wait(0);
			}

			free(commands[0]);
		}
		else if(!strcmp(commands[0], "vi") || !strcmp(commands[0], "vim")){
			if(!strcmp(commands[0], "vi")){
				commands[0] = (char*)malloc(LEN*4);
				strcpy(commands[0], "/usr/bin/vi");
			}
			else if(!strcmp(commands[0], "vim")){
				commands[0] = (char*)malloc(LEN*4);
				strcpy(commands[0], "/usr/bin/vim");
			}

			pid_t pid = fork();
			if(pid==0){
				execv(commands[0], commands);
				exit(0);
			}
			else if(pid>0){
				wait(0);
			}

			free(commands[0]);
		}
		else if(!strncmp(commands[0], "exit", 4)){
			exit(0);
		}
		else{
			pid_t pid = fork();
			if(pid==0){
				execl("help", "help", NULL);
				exit(0);
			}
			else if(pid>0){
				wait(0);
			}
		}
	}

	free(backupDir);
}

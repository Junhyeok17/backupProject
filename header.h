#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include "hash.h"
#include "stack.h"

#define PATH 4096
#define NAME 255

char *hashfunc;			// 어떤 해쉬 함수 쓸 건지 저장
char *backupDir;		// 백업 디렉터리 경로 저장

// 백업한 파일을 연결 리스트로 관리하기 위한 구조체
typedef struct File{
	char *backupPath;	// 백업 파일 경로 저장
	char *originPath;	// 기존 파일 경로 저장
	char *hash;			// 해당 파일 내용 해쉬값 저장
	char *time;			// 백업한 시간대 저장
	struct stat status;
	struct File *prev;
	struct File *next;
}File;

File *head = NULL;

// 백업하려는 파일 이름 뒤에 백업하는 현재 시간 이어붙이기 위해 현재시간 불러오기
char* getCurrentTime()
{
	struct tm *tm_p;
	time_t now;

	time(&now);
	tm_p = localtime(&now);

	char timestamp[PATH];
	sprintf(timestamp, "_%02d%02d%02d%02d%02d%02d", (tm_p->tm_year+1900)%100, tm_p->tm_mon+1,
			tm_p->tm_mday, tm_p->tm_hour, tm_p->tm_min, tm_p->tm_sec);

	char *curtime = (char*)malloc(strlen(timestamp)+1);
	strcpy(curtime, timestamp);
	return curtime;
}

// 백업 디렉터리의 파일들 백업 파일 관리하는 연결리스트에 추가
void addFile(char *filepath)
{
	File *tmp = head;
	if(!tmp){
		head = (File*)malloc(sizeof(File));
		head->next = NULL;
		head->prev = NULL;
		tmp = head;
	}
	else{
		while(tmp->next)
			tmp = tmp->next;

		tmp->next = (File*)malloc(sizeof(File));
		tmp->next->prev = tmp;
		tmp = tmp->next;
		tmp->next = NULL;
	}

	// 파일의 백업 경로 저장
	tmp->backupPath = (char*)malloc(strlen(filepath)+1);
	strcpy(tmp->backupPath, filepath);

	int i;
	for(i=strlen(filepath)-1;i>=0;i--)
		if(filepath[i]=='_')
			break;

	// 파일의 기존 경로 저장
	tmp->originPath = (char*)malloc(strlen(filepath)+1);
	strncpy(tmp->originPath, filepath, i);
	tmp->originPath[i] = '\0';

	tmp->time = (char*)malloc(20);
	strcpy(tmp->time, filepath+i+1);
	
	int len = strlen(backupDir);
	int homeLen = len-7;
	strcpy(tmp->originPath+homeLen, tmp->originPath+len);

	struct stat status;
	lstat(tmp->backupPath, &status);

	tmp->status = status;

	// 파일의 해쉬값 저장
	if(!strcmp(hashfunc, "md5"))
		tmp->hash = hash_md5(tmp->backupPath);
	else
		tmp->hash = hash_sha(tmp->backupPath);
}

// 백업 디렉터리 하위 파일 중 디렉터리 존재 시 open 해서 내부 파일을 연결리스트에 추가
void scanFile(char *filepath)
{
	int num;
	struct dirent **list;

	if((num=scandir(filepath, &list, NULL, alphasort)) < 0){
		fprintf(stderr, "scandir error\n\n");
		exit(1);
	}

	for(int i=0;i<num;i++){
		if(!strcmp(list[i]->d_name, ".") || !strcmp(list[i]->d_name, ".."))
			continue;

		char *newfile = (char*)malloc(PATH);
		strcpy(newfile, filepath);
		strcat(newfile, "/");
		strcat(newfile, list[i]->d_name);

		struct stat status;
		lstat(newfile, &status);

		if(S_ISREG(status.st_mode))
			addFile(newfile);
		else if(S_ISDIR(status.st_mode))
			scanFile(newfile);

		free(newfile);
	}
}

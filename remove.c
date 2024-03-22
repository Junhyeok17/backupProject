#include "header.h"
#include <errno.h>

#define PATH 4096
#define NAME 255

int opta = 0;			// a 옵션 적용 여부 확인
int optc = 0;			// c 옵션 적용 여부 확인
int fault = 0;			// 옵션 제대로 적용했는지 확인

int file = 0;			// c 옵션 적용 시 삭제한 정규파일 수 확인
int directory = 0;		// c 옵션 적용 시 삭제한 디렉터리 수 확인
int fromDirectory = 0;	// 디렉터리 함수를 통해 정규파일 삭제하는지, 정규파일만 삭제하는지 확인
int isRegOrDir = 0;		// 백업 디렉터리에만 존재하는 파일이 정규파일인지 디렉터리인지 구분

char* getPathNoHome(char*);	// 삭제하려는 백업 파일이 홈디렉터리에는 없고 백업 디렉터리에는 있을 때
// 백업 디렉터리를 통해 지우려는 파일의 기존 절대 경로 구하기
void removeAll(char*);			// c 옵션 적용 시 모든 파일 지우는 함수
void removeFile(char*);			// 정규 파일 지우는 함수
void removeSomeFiles(char*);	// 특정 디렉터리 하위 파일 지우는 함수
void bubbleSortByTime();		// 백업 디렉터리 내부 파일들 백업한 시간순으로 정렬하는 함수
void printFileSize(off_t);		// 정규파일들 크기 출력하는 함수

int main(int argc, char *argv[])
{
	// 백업 디렉터리 경로 저장
	backupDir = (char*)malloc(strlen(getenv("HOME"))+10);
	strcpy(backupDir, getenv("HOME"));
	strcat(backupDir, "/backup");

	hashfunc = argv[argc-1];

	// 백업 디렉터리 내부 파일들 연결리스트로 만들기
	scanFile(backupDir);
	// 연결리스트로 만든 파일들 백업한 시간 순으로 정렬
	bubbleSortByTime();

	argc--;

	if(!(argc==2 || argc==3)){		// 입력 인자가 일치하지 않을 시
		printf("Usage : remove <FILENAME> [OPTION]\n");
		printf("  -a : remove all file(recursive)\n");
		printf("  -c : clear backup directory\n\n");
		exit(0);
	}

	char *filename = argv[1];

	int error = 0;

	char opt;
	opterr = 0;

	optind = 0;
	while((opt=getopt(argc, argv, "ac"))!=-1){		// 옵션 정확한지 확인
		switch(opt){
			case 'a':
				opta = 1;
				break;
			case 'c':
				optc = 1;
				break;
			default:
				fault = 1;
				break;
		}
	}
	optind = 0;

	int noFileCheck = 0;	// 홈디렉터리에 없는 백업 파일인지 확인
	if(argv[1][0]=='/'){			// 파일 절대 경로로 변환 및 존재 여부 확인
		char tmppath[PATH];
		if(access(filename, F_OK)){
			noFileCheck = 1;
			strcpy(tmppath, backupDir);
			strcat(tmppath, filename+strlen(getenv("HOME")));
			if(access(tmppath, F_OK)){		// 정규파일인지 확인
				File *ftmp = head;
				while(ftmp){
					if(!strcmp(ftmp->originPath, tmppath))
						break;
					ftmp = ftmp->next;
				}
				if(ftmp){
					isRegOrDir = 0;
					filename = ftmp->originPath;
				}
				else
					filename = NULL;
			}					// 존재하는데 정규파일이 아니면 디렉터리
			else
				isRegOrDir = 1;
		}

		if(!filename){
			printf("Usage : remove <FILENAME> [OPTION]\n");
			printf("  -a : remove all file(recursive)\n");
			printf("  -c : clear backup directory\n\n");
			exit(0);
		}

		char *tmp = filename;
		filename = (char*)malloc(strlen(filename)+1);
		strcpy(filename, tmp);
	}
	else{
		char *curfilename = filename;
		if(!optc)
			filename = realpath(filename, NULL);

		if(!filename){		// 삭제하려는 파일이 홈디렉토리에는 없지만 백업디렉토리에는 있는 경우
			filename = getPathNoHome(curfilename);
			if(filename)
				noFileCheck = 1;
		}

		if(!filename && !optc){
			printf("Usage : remove <FILENAME> [OPTION]\n");
			printf("  -a : remove all file(recursive)\n");
			printf("  -c : clear backup directory\n\n");
			exit(0);
		}
	}

	if(fault || (opta && optc)){	// 잘못된 옵션이나 a, c 옵션 동시 입력 여부 확인
		printf("Usage : remove <FILENAME> [OPTION]\n");
		printf("  -a : remove all file(recursive)\n");
		printf("  -c : clear backup directory\n\n");
		exit(0);
	}

	if(optc){		// c 옵션 사용한 경우
		if(head){		// 백업 디렉터리 내부가 존재하는 경우
			while(head){
				File *tmp = head;
				head = head->next;
				free(tmp->backupPath);	free(tmp->originPath);
				free(tmp->hash);	free(tmp);
			}
			// 모든 파일 지우기
			removeAll(backupDir);
			head = NULL;

			printf("backup directory cleared(%d regular files and %d subdirectories totally).\n\n", file, directory);
			exit(0);
		}
		else{			// 백업 디렉터리가 비어있는 경우
			printf("no file(s) in the backup\n\n");
			exit(0);
		}
	}

	char homedir[PATH]; strcpy(homedir, getenv("HOME"));
	if(strncmp(filename, homedir, strlen(homedir))){	// 경로가 홈 디렉터리 벗어나는지 확인
		fprintf(stderr, "\"%s\" can't be backuped\n\n", filename);
		free(filename);
		exit(0);
	}

	if(!strncmp(filename, backupDir, strlen(backupDir))){	// 경로가 백업 디렉토리 포함하는지 확인
		fprintf(stderr, "\"%s\" can't be backuped\n\n", filename);
		free(filename);
		exit(0);
	}

	struct stat status;
	stat(filename, &status);

	if(strlen(filename) > PATH)			// 파일 길이 확인
		error = 1;

	if(!noFileCheck){
		if(access(filename, R_OK)==-1 || access(filename, W_OK)==-1)		// 파일 접근 권한 확인
			error = 2;

		if(!(S_ISREG(status.st_mode) || S_ISDIR(status.st_mode)))	// 정규 파일 혹은 디렉터리 여부 확인
			error =3;
	}
	else{
		if(status.st_mode==0){
			if(!isRegOrDir)
				status.st_mode=S_IFREG;
			else
				status.st_mode=S_IFDIR;
		}
	}

	if(S_ISDIR(status.st_mode))	// 인자가 디렉터리일 때 a 옵션 확인
		if(!opta)
			error = 4;

	switch(error){
		case 1:
			printf("파일 길이가 제한을 넘었습니다.\n\n");
			free(filename);
			exit(0);
		case 2:
			printf("파일에 대한 접근 권한이 없습니다.\n\n");
			free(filename);
			exit(0);
		case 3:
			printf("정규 파일 또는 디렉터리가 아닙니다.\n\n");
			free(filename);
			exit(0);
		case 4:
			printf("디렉토리에 -a 옵션이 적용되지 않았습니다.\n\n");
			free(filename);
			exit(0);
	}

	if(S_ISREG(status.st_mode)){
		// 삭제하려는 파일이 정규파일인 경우
		removeFile(filename);
		free(filename);
	}
	else if(S_ISDIR(status.st_mode) && opta){
		// 삭제하려는 파일이 디렉터리인 경우
		fromDirectory = 1;
		removeSomeFiles(filename);
		free(filename);
	}

	free(backupDir);
	exit(0);
}

char* getPathNoHome(char *filename)
{
	char *curname = filename;
	char originpwd[PATH];
	char backuppwd[PATH];

	getcwd(originpwd, PATH);
	strcpy(backuppwd, backupDir);
	strcat(backuppwd, originpwd+strlen(getenv("HOME")));
	chdir(backuppwd);

	char lastpath[PATH], tmp[PATH];

	int i;
	for(i=strlen(filename)-1;i>=0;i--)
		if(filename[i]=='/')
			break;

	if(i>0){			// '/' 가 경로에 포함된 경우
		strncpy(lastpath, filename, i);
		lastpath[i]='\0';
		char *backuppath = realpath(lastpath, NULL);
		if(!backuppath)
			return NULL;
		strcpy(tmp, backuppath);
		strcat(tmp, filename+i);
		strcpy(lastpath, getenv("HOME"));
		strcat(lastpath, tmp+strlen(backupDir));

		File *ftmp = head;		// 경로가 정규파일인지 디렉터리인지 확인
		while(ftmp){
			if(!strcmp(ftmp->originPath, lastpath))
				break;
			ftmp = ftmp->next;
		}

		if(!ftmp){
			if(access(tmp, F_OK))
				return NULL;
			else{
				isRegOrDir = 1;
			}
		}
		else{
			isRegOrDir = 0;
			strcpy(lastpath, ftmp->originPath);
		}
	}
	else{			// '/'가 경로에 포함되지 않은 경우
		char *backuppath = realpath(filename, NULL);

		if(backuppath){
			strncpy(lastpath, backuppath, strlen(getenv("HOME")));
			lastpath[strlen(getenv("HOME"))] = '\0';
			strcat(lastpath, backuppath+strlen(backupDir));
			isRegOrDir = 1;
		}
		else{
			strcpy(tmp, originpwd);
			strcat(tmp, "/");
			strcat(tmp, filename);

			File *ftmp = head;	// 경로가 정규파일인지 디렉터리인지 확인
			while(ftmp){
				if(!strcmp(ftmp->originPath, tmp))
					break;
				ftmp = ftmp->next;
			}

			if(!ftmp)
				return NULL;
			else{
				isRegOrDir = 0;
				strcpy(lastpath, ftmp->originPath);
			}
		}
	}

	char *result = (char*)malloc(strlen(lastpath)+1);
	strcpy(result, lastpath);
	return result;
}

void removeAll(char *pathname)
{
	struct dirent **list;
	int num = 0;

	if((num=scandir(pathname, &list, NULL, alphasort)) < 0){
		fprintf(stderr, "scandir error\n\n");
		exit(1);
	}

	// 백업디렉터리 내에 있는 파일들 재귀 탐색으로 모두 지우기
	for(int i=0;i<num;i++){
		if(!strcmp(list[i]->d_name, ".") || !strcmp(list[i]->d_name, ".."))
			continue;

		char *newfile = (char*)malloc(strlen(pathname)+strlen(list[i]->d_name)+10);
		strcpy(newfile, pathname);
		strcat(newfile, "/");
		strcat(newfile, list[i]->d_name);

		struct stat status;
		stat(newfile, &status);

		if(S_ISREG(status.st_mode)){
			remove(newfile);
			free(newfile);
			file++;
		}
		else if(S_ISDIR(status.st_mode)){
			removeAll(newfile);
			remove(newfile);
			free(newfile);
			directory++;
		}
	}
}

void removeFile(char *filename)
{
	File *dest = NULL, *tmp = head;

	if(!opta || fromDirectory){			// a 옵션 없거나 디렉토리 하위 삭제일 때
		int idx = 0;
		while(tmp){				// 경로 같은 파일 개수 구하기
			if(!strcmp(tmp->originPath, filename)){
				if(!dest)
					dest = tmp;
				idx++;
			}
			tmp = tmp->next;
		}

		if(!idx){
			printf("no such backup file\n\n");
			return;
		}
		if(idx==1){			// 경로 같은 파일 한 개일 때
			printf("\"%s\" backup file removed\n\n", dest->backupPath);

			if(dest==head){	// 지우려는 파일이 첫 파일일 때
				head = head->next;
			}
			else if(dest->next==NULL){		// 지우려는 파일이 마지막 파일일 때
				dest->prev->next = NULL;
			}
			else{		// 지우려는 파일이 중간 파일일 때
				dest->prev->next = dest->next;
				dest->next->prev = dest->prev;
			}

			remove(dest->backupPath);
			if(!fromDirectory){
			free(dest->originPath);	free(dest->backupPath);
			free(dest->time);	free(dest->hash);	free(dest);
			}
		}
		else{				// 경로 같은 파일 여러 개일 때
			tmp = head;
			idx = 0;
			printf("backup file list of \"%s\"\n", filename);
			printf("%d. exit\n", idx++);
			while(tmp){				// 경로 같은 파일 시간대 순으로 출력
				if(!strcmp(tmp->originPath, filename)){
					if(!dest)
						dest = tmp;
					printf("%d. %s\t\t", idx++, tmp->time);
					stackInit();		// 숫자마다 콤마 찍기에 사용되는 스택 초기화
					printFileSize(tmp->status.st_size);
					printf("bytes\n");
				}
				tmp = tmp->next;
			}

			int num;	// 선택한 번호
			while(1){	// 잘못된 입력값이 들어왔을 경우 에러 처리
				printf("Choose file to remove\n>> ");
				char choice[100]; fgets(choice, 100, stdin);
				choice[strlen(choice)-1] = '\0';

				int check = 1;
				for(int i=0;i<strlen(choice);i++)
					if(!('0' <= choice[i] && choice[i] <= '9'))
						check = 0;

				if(check){
					num = atoi(choice);
					if(0<=num && num<idx)
						break;
				}
				printf("잘못된 입력값을 입력했습니다.\n\n");
			}

			if(!num)
				return;

			tmp = dest;
			for(int i=0;i<num-1;i++)
				tmp = tmp->next;

			if(!tmp->prev && !tmp->next && tmp==head)
				head = NULL;

			if(!tmp->prev)		// 첫 파일 지우려는 경우
				head = head->next;
			else if(!tmp->next)		// 마지막 파일 지우려는 경우
				tmp->prev->next = tmp->next;
			else{			// 중간 파일 지울 경우
				tmp->prev->next = tmp->next;
				tmp->next->prev = tmp->prev;
			}
			printf("\"%s\" backup file removed\n\n", tmp->backupPath);
			remove(tmp->backupPath);
			if(!fromDirectory){
			free(tmp->originPath);	free(tmp->backupPath);
			free(tmp->time);	free(tmp->hash);	free(tmp);
			}
		}
	}
	else{		// a 옵션 있을 때
		File *dest = NULL;	// 삭제할 첫 파일 저장
		int nums = 0; // 기존경로가 삭제할 파일 경로와 일치하는 파일들 개수 저장
		while(tmp){				// 경로 같은 파일 개수 구하기
			if(!strcmp(tmp->originPath, filename)){
				if(!dest)
					dest = tmp;
				nums++;
			}
			tmp = tmp->next;
		}

		if(!nums){
			printf("no such backup files\n\n");
			return;
		}

		tmp = dest;
		File *delList[nums];
		nums = 0;

		while(tmp){
			if(strcmp(tmp->originPath, filename))
				break;
			delList[nums++] = tmp;
			tmp = tmp->next;
		}

		for(int i=0;i<nums;i++)
			printf("\"%s\" backup file removed\n", delList[i]->backupPath);

		if(delList[0]==head && !tmp){
			// 지우려는 파일 리스트가 모든 파일일 때
			head = NULL;
		}
		else if(delList[0]==head && tmp){
			// 지우려는 파일 리스트가 첫 파일을 포함할 때
			head = tmp;
		}
		else if(!tmp){
			// 지우려는 파일 리스트가 마지막 노드를 포함할 때
			tmp = delList[0]->prev;
			tmp->next = NULL;
		}
		else{
			// 지우려는 파일 리스트가 중간 파일들일 때
			tmp = delList[0]->prev;
			tmp->next = delList[nums-1]->next;
		}
		for(int i=0;i<nums;i++){
			tmp = delList[i];
			remove(tmp->backupPath);
			free(tmp->originPath);	free(tmp->backupPath);
			free(tmp->time);	free(tmp->hash);	free(tmp);
		}
	}
}

void removeSomeFiles(char *filename)
{
	File *tmp = head, *dest = NULL;

	int nums = 0;
	int len;
	// 하위 파일에 해당되는 파일들 전부 찾기
	while(tmp){
		len = strlen(filename);
		if(strlen(tmp->originPath) < len){
			tmp = tmp->next;
			continue;
		}
		if(!strncmp(tmp->originPath, filename, len) && tmp->originPath[len]=='/'){
			if(!dest)
				dest = tmp;
			nums++;
		}
		tmp = tmp->next;
	}

	if(!nums){
		printf("삭제하려는 백업디렉토리가 비어있습니다.\n\n");
		return;
	}

	tmp = dest;
	File *delList[nums];
	nums = 0;

	len = strlen(filename);
	while(tmp){
		if(!strncmp(tmp->originPath, filename, len) && tmp->originPath[len]=='/')
			delList[nums++] = tmp;
		else
			break;
		tmp = tmp->next;
	}

	File *compList[nums];
	int compIdx = 0;

	for(int i=0;i<nums;i++){
		if(compIdx && !strcmp(compList[compIdx-1]->originPath, delList[i]->originPath))
			continue;
		compList[compIdx++] = delList[i];
		removeFile(delList[i]->originPath);
	}

	if(delList[0]==head && !tmp){
		// 지우려는 파일 리스트가 모든 파일일 때
		head = NULL;
	}
	else if(delList[0]==head && tmp){
		// 지우려는 파일 리스트가 첫 파일을 포함할 때
		head = tmp;
	}
	else if(!tmp){
		// 지우려는 파일 리스트가 마지막 노드를 포함할 때
		tmp = delList[0]->prev;
		tmp->next = NULL;
	}
	else{
		// 지우려는 파일 리스트가 중간 파일들일 때
		tmp = delList[0]->prev;
		tmp->next = delList[nums-1]->next;
	}

	for(int i=0;i<nums;i++){
		tmp = delList[i];
		free(tmp->originPath);	free(tmp->backupPath);
		free(tmp->time);	free(tmp->hash);	free(tmp);
	}
}

void bubbleSortByTime()
{
	// 백업 디렉터리에 파일 존재 시 백업한 시간 순으로 정렬
	if(!head)
		return;
	for(File *firsthead = head;firsthead->next!=NULL;firsthead = firsthead->next){
		for(File *secondhead = firsthead->next;secondhead!=NULL;secondhead = secondhead->next){
			if(strcmp(firsthead->backupPath, secondhead->backupPath) > 0){
				char *tmp = firsthead->backupPath;
				firsthead->backupPath = secondhead->backupPath;
				secondhead->backupPath = tmp;

				tmp = firsthead->originPath;
				firsthead->originPath = secondhead->originPath;
				secondhead->originPath = tmp;

				tmp = firsthead->hash;
				firsthead->hash = secondhead->hash;
				secondhead->hash = tmp;

				tmp = firsthead->time;
				firsthead->time = secondhead->time;
				secondhead->time = tmp;

				struct stat status = firsthead->status;
				firsthead->status = secondhead->status;
				secondhead->status = status;
			}
		}
	}
}

void printFileSize(off_t size)
{
	if(!size){
		printf("0");
		return;
	}

	int nums = stackSize();
	while(size){
		stackAdd(size%10);
		size/=10;
		nums++;
	}

	// 파일 크기 1000 단위로 , 찍기
	while(stackSize()){
		if(nums!=stackSize() && stackSize()%3==0)
			printf(",");
		printf("%d", stackPop());
	}
}

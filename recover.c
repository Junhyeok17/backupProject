#include "header.h"
#include <errno.h>

#define PATH 4096
#define NAME 255

int optd = 0;					// d 옵션 적용 여부 확인
int optn = 0;					// n 옵션 적용 여부 확인
int fault = 0;					// 옵션 제대로 적용했는지 확인
int fromDirectory = 0;			// 디렉터리부터 탐색했는지 확인
int isRegOrDir = 0;				// 정규파일 또는 디렉터리 구분

char *originfilename = NULL;	// 원래 경로
char *newfilename = NULL;		// n 옵션 적용 후 경로
char *newname = NULL;			// 새로 적용할 경로 이름

char* getPathNoHome(char*);	// 복구하려는 백업 파일이 홈디렉터리에는 없고 백업 디렉터리에는 있을 때
// 백업 디렉터리를 통해 복구하려는 파일의 기존 절대 경로 구하기
char* getNewPath(char*);			// n 옵션 적용 시 새로운 경로 리턴해주는 함수
void recoverFile(char*, char*, int);	// 복구하려는 파일 홈디렉터리에 복구하는 함수
void recoverList(char*);			// 복구 파일 찾거나 리스트로 출력하는 함수
void recoverDirectory(char*);		// 특정 디레겉리 복구하는 함수
void bubbleSortByTime();			// 연결리스트로 만든 백업 파일들 백업된 시간 순으로 정렬
void printFileSize(off_t);			// 파일 크기 출력
int dupCheck(char*, char*);			// 파일 중복 여부 확인

int main(int argc, char *argv[])
{
	// 백업 디렉터리 경로 저장
	backupDir = (char*)malloc(strlen(getenv("HOME"))+10);
	strcpy(backupDir, getenv("HOME"));
	strcat(backupDir, "/backup");

	hashfunc = argv[argc-1];

	// 백업 디렉터리 탐색해서 연결리스트 만들기
	scanFile(backupDir);
	// 연결리스트로 만든 백업 파일들 백업된 시간순으로 정렬
	bubbleSortByTime();

	argc--;

	if(!(2<=argc && argc<=5)){		// 입력 인자가 일치하지 않을 시
		printf("Usage : recover <FILENAME> [OPTION]\n");
		printf("  -d : recover directory recursive\n");
		printf("  -n <NEWNAME> : recover file with new name\n\n");
		exit(0);
	}

	char *filename = argv[1];

	int error = 0;

	char opt;
	opterr = 0;

	optind = 0;
	while((opt=getopt(argc, argv, "dn:"))!=-1){		// 옵션 정확한지 확인
		switch(opt){
			case 'd':
				optd = 1;
				break;
			case 'n':
				optn = 1;
				newname = optarg;
				break;
			default:
				fault = 1;
				break;
		}
	}
	optind = 0;

	int noFileCheck = 0;	// 홈디렉터리에 없는 백업 파일인지 확인
	if(filename[0]=='/'){
		char tmppath[PATH];
		if(access(filename, F_OK)){		// 홈디렉터리에 없는 경우
			noFileCheck = 1;
			strcpy(tmppath, backupDir);
			strcat(tmppath, filename+strlen(getenv("HOME")));
			if(access(tmppath, F_OK)){	// 디렉터리면 그대로. 아니면 정규파일인지 확인
				File *ftmp = head;
				while(ftmp){			// 여기에 있으면 정규파일
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
			}
			else					// 아니면 디렉터리
				isRegOrDir = 1;
		}

		if(!filename){
			printf("Usage : recover <FILENAME> [OPTION]\n");
			printf("  -d : recover directory recursive\n");
			printf("  -n <NEWNAME> : recover file with new name\n\n");
			exit(0);
		}

		char *tmp = filename;
		filename = (char*)malloc(strlen(filename)+1);
		strcpy(filename, tmp);
	}
	else{
		char *curfilename = filename;
		filename = realpath(filename, NULL);

		if(!filename){		// 삭제하려는 파일이 홈디렉토리에는 없지만 백업디렉토리에는 있는 경우
			filename = getPathNoHome(curfilename);
			noFileCheck = 1;
		}

		if(!filename){
			printf("Usage : recover <FILENAME> [OPTION]\n");
			printf("  -d : recover directory recursive\n");
			printf("  -n <NEWNAME> : recover file with new name\n\n");
			exit(0);
		}

	}

	if(fault || (optn && !newname)){	// 잘못된 옵션이나 n 옵션 사용 시, 파일 이름 입력 여부 확인
		printf("Usage : recover <FILENAME> [OPTION]\n");
		printf("  -d : recover directory recursive\n");
		printf("  -n <NEWNAME> : recover file with new name\n\n");
		exit(0);
	}

	char homedir[PATH]; strcpy(homedir, getenv("HOME"));

	char homePath[PATH];
	strcpy(homePath, homedir);
	strcat(homePath, filename+strlen(backupDir)-2);

	if(strncmp(homePath, homedir, strlen(homedir))){	// 경로가 홈 디렉터리 벗어나는지 확인
		fprintf(stderr, "\"%s\" can't be backuped\n\n", filename);
		free(filename);
		exit(0);
	}

	if(!strncmp(homePath, backupDir, strlen(backupDir))){	// 경로가 백업 디렉토리 포함하는지 확인
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

	if(S_ISDIR(status.st_mode))	// 인자가 디렉터리일 때 d 옵션 확인
		if(!optd)
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
			printf("디렉토리에 -d 옵션이 적용되지 않았습니다.\n\n");
			free(filename);
			exit(0);
	}

	if(optn){
		// n 옵션 사용 시 전역변수로 활용할 기존 경로명과 새 경로명
		originfilename = (char*)malloc(strlen(filename)+1);
		strcpy(originfilename, filename);
		int i;
		for(i=strlen(filename)-1;i>=0;i--)
			if(filename[i]=='/')
				break;

		newfilename = (char*)malloc(PATH);
		strncpy(newfilename, originfilename, i);
		newfilename[i]='\0';
		strcat(newfilename, "/");
		strcat(newfilename, newname);
	}

	if(S_ISREG(status.st_mode)){
		// 정규 파일 복구 시
		recoverList(filename);
		free(filename);
	}
	else if(S_ISDIR(status.st_mode) && optd){
		// 디렉터리 복구 시 옵션 동시 확인
		fromDirectory = 1;
		recoverDirectory(filename);
		free(filename);
	}

	free(backupDir);
	if(originfilename)
		free(originfilename);
	if(newfilename)
		free(newfilename);
	exit(0);
}

char* getPathNoHome(char *filename)
{
	char *curname = filename;
	char originpwd[PATH];	// 원래 작업 디렉터리
	char backuppwd[PATH];	// 백업 디렉터리 기준 작업 디렉터리
	getcwd(originpwd, PATH);
	strcpy(backuppwd, backupDir);
	strcat(backuppwd, originpwd+strlen(getenv("HOME")));
	chdir(backuppwd);

	char lastpath[PATH], tmp[PATH];

	int i;
	for(i=strlen(filename)-1;i>=0;i--)
		if(filename[i]=='/')
			break;

	if(i>0){		// '/' 있는 경우 백업 디렉터리에서 절대경로 찾기
		strncpy(lastpath, filename, i);
		lastpath[i] = '\0';
		char *backuppath = realpath(lastpath, NULL);
		if(!backuppath)
			return NULL;
		strcpy(tmp, backuppath);
		strcat(tmp, filename+i);
		strcpy(lastpath, getenv("HOME"));
		strcat(lastpath, tmp+strlen(backupDir));
		// 백업 디렉터리 이용해서 기존 디렉터리 절대 경로 얻고 존재하는지 확인
		File *ftmp = head;
		while(ftmp){	// 해당 경로가 정규파일인지 확인
			if(!strcmp(ftmp->originPath, lastpath))
				break;
			ftmp = ftmp->next;
		}

		if(!ftmp){		// 정규 파일이 아닐 때 디렉터리인지 확인
			if(access(tmp, F_OK))	// 디렉터리도 아니면 존재 안 하는 것
				return NULL;
			else{
				isRegOrDir = 1;
			}
		}
		else{		// 정규 파일일 때
			isRegOrDir = 0;
			strcpy(lastpath, ftmp->originPath);
		}
	}
	else{			// '/' 없는 경우 백업 디렉터리에서 절대경로 찾기
		char *backuppath = realpath(filename, NULL);
		if(backuppath){		// 디렉터리인 경우
			strncpy(lastpath, backuppath, strlen(getenv("HOME")));
			lastpath[strlen(getenv("HOME"))] = '\0';
			strcat(lastpath, backuppath+strlen(backupDir));
			isRegOrDir = 1;
		}
		else{				// 디렉터리가 아닌 경우
			// 백업 파일 리스트의 기존 절대 경로와 비교하여 정규 파일인지 확인
			strcpy(tmp, originpwd);
			strcat(tmp, "/");
			strcat(tmp, filename);

			File *ftmp = head;
			while(ftmp){	// 해당 경로가 정규파일인지 확인
				if(!strcmp(ftmp->originPath, tmp))
					break;
				ftmp = ftmp->next;
			}

			if(!ftmp){		// 백업 디렉터리에도 존재하지 않을 경우
				return NULL;
			}
			else{			// 백업 디렉터리에는 있는 정규 파일일 경우
				isRegOrDir = 0;
				strcpy(lastpath, ftmp->originPath);
			}
		}
	}
	chdir(originpwd);
	char *result = (char*)malloc(strlen(lastpath)+1);
	strcpy(result, lastpath);
	return result;
}

char* getNewPath(char *originPath)
{
	// n 옵션 적용한 경우 새로운 경로 만들어서 리턴
	char *newPath = (char*)malloc(strlen(originfilename)+strlen(newfilename)+strlen(originPath)+1);
	strcpy(newPath, newfilename);
	strcat(newPath, originPath+strlen(originfilename));
	return newPath;
}

// 백업 파일 중 디렉터리 기준 탐색
void recoverDirectory(char *filename)
{
	fromDirectory = 1;
	File *tmp = head, *dest = NULL;

	int nums = 0;
	int len = strlen(filename);

	// 특정 디렉터리의 하위 파일들 개수 세기
	while(tmp){
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
		printf("복구하려는 백업 디렉터리가 비어있습니다.\n\n");
		return;
	}

	tmp = dest;
	File *delList[nums];
	nums = 0;

	while(tmp){
		if(!strncmp(tmp->originPath, filename, len) && tmp->originPath[len]=='/')
			delList[nums++] = tmp;
		else
			break;
		tmp = tmp->next;
	}

	File *compList[nums];
	int compIdx = 0;
	// 해당하는 파일들 복구
	for(int i=0;i<nums;i++){
		char *newpath;
		// n 옵션 적용한 경우 새로운 경로 리턴 받기
		if(optn)
			newpath = getNewPath(delList[i]->originPath);
		else
			newpath = delList[i]->originPath;
		if(!dupCheck(delList[i]->backupPath, newpath)){
			if(compIdx && !strcmp(compList[compIdx-1]->originPath, newpath))
				continue;
			compList[compIdx++] = delList[i];
			recoverList(delList[i]->originPath);
		}
		if(optn)
			free(newpath);
	}
	if(optd){
		for(int j=0;j<compIdx;j++){
			free(compList[j]->backupPath); free(compList[j]->originPath);
			free(compList[j]->time); free(compList[j]->hash); free(compList[j]);
		}
	}
}

void recoverList(char *filename)
{
	File *dest = NULL, *tmp = head;
	int idx = 0;

	while(tmp){		// 경로 같은 파일 개수 구하기
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
	else if(idx==1){		// 겹치는 파일이 하나일 때
		char *newpath = filename;
		if(optn)
			newpath = getNewPath(filename);
		if(!dupCheck(dest->backupPath, newpath)){
			if(dest==head) // 복구하려는 파일이  첫 파일일 때
				head = head->next;
			else if(dest->next==NULL)	// 복구하려는 파일이 마지막 파일일 때
				dest->prev->next = NULL;
			else{
				dest->prev->next = dest->next;
				dest->next->prev = dest->prev;
			}

			recoverFile(dest->backupPath, newpath, strlen(getenv("HOME"))+2);
			remove(dest->backupPath);
			if(!optd){
				free(dest->backupPath); free(dest->originPath);
				free(dest->time); free(dest->hash); free(dest);
			}
		}
	}
	else{		// 겹치는 파일이 여러 개일 때
		tmp = head;
		idx = 0;
		printf("backup file list of \"%s\"\n", filename);
		printf("%d. exit\n", idx++);

		while(tmp){		// 경로 같은 파일 시간대 순으로 출력
			if(!strcmp(tmp->originPath, filename)){
				if(!dest)
					dest = tmp;
				printf("%d. %s\t", idx++, tmp->time);
				stackInit();
				printFileSize(tmp->status.st_size);
				printf("bytes\n");
			}
			tmp = tmp->next;
		}

		int num;

		while(1){
			printf("Choose file to recover\n>> ");
			char choice[100]; fgets(choice, 100, stdin);
			choice[strlen(choice)-1] = '\0';

			int check = 1;
			for(int i=0;i<strlen(choice);i++)
				if(!('0'<=choice[i] && choice[i]<='9'))
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

		char *newpath = filename;
		if(optn)
			newpath = getNewPath(filename);

		if(!dupCheck(tmp->backupPath, newpath)){
			// 파일이 하나만 있는 경우
			if(!tmp->prev && !tmp->next && tmp==head)
				head = NULL;

			if(!tmp->prev)
				head = head->next;
			else if(!tmp->next)
				tmp->prev->next = tmp->next;
			else{
				tmp->prev->next = tmp->next;
				tmp->next->prev = tmp->prev;
			}

			dest = tmp;
			recoverFile(dest->backupPath, newpath, strlen(getenv("HOME"))+2);
			remove(dest->backupPath);
			if(!optd){
				free(dest->backupPath); free(dest->originPath);
				free(dest->time); free(dest->hash); free(dest);
			}
		}
	}
}

void recoverFile(char *backupfile, char *recoverfile, int idx)
{
	while(1){
		//	파일 생성을 위한 디렉토리 생성
		if(recoverfile[idx]=='\0')
			break;

		int fd;
		char tmppath[PATH];

		while(recoverfile[idx]!='/' && recoverfile[idx]!='\0')
			idx++;

		if(recoverfile[idx]!='\0'){
			strncpy(tmppath, recoverfile, idx);
			tmppath[idx] = '\0';

			if(!access(tmppath, F_OK)){
				idx++;
				continue;
			}
			else if((fd=mkdir(tmppath, 0777)) < 0){
				fprintf(stderr, "creat directory error for %s\n", tmppath);
				exit(0);
			}
			else
				idx++;
		}
	}

	int fd_read, fd_write;

	// 백업디렉터리에서 기존 디렉터리로 파일 복구
	if((fd_write=open(recoverfile, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0){
		fprintf(stderr, "open write %s file error\n\n", recoverfile);
		perror("open file error");
		exit(1);
	}
	else{
		char buf[1024];
		int size;

		if((fd_read=open(backupfile, O_RDONLY)) < 0){
			fprintf(stderr, "open read %s file error\n\n", backupfile);
			perror("open file error");
			exit(1);
		}
		else{
			while((size=read(fd_read, buf, 1024)) > 0)
				write(fd_write, buf, size);
		}

		printf("\"%s\" backup recover to \"%s\"\n\n", backupfile, recoverfile);
	}
	close(fd_read);
	close(fd_write);
	if(optn)
		free(recoverfile);
}

void bubbleSortByTime()
{
	// 연결리스트로 만들어진 백업 파일들 백업된 시간순으로 정렬
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

	while(stackSize()){
		if(nums!=stackSize() && stackSize()%3==0)
			printf(",");
		printf("%d", stackPop());
	}
}

int dupCheck(char *backuppath, char *filepath)
{
	// 두 파일의 내용이 중복되는지 확인
	char *hash1 = NULL;
	char *hash2 = NULL;

	if(!strcmp(hashfunc, "md5")){
		hash1 = hash_md5(backuppath);
		hash2 = hash_md5(filepath);
	}
	else{
		hash1 = hash_sha(backuppath);
		hash2 = hash_sha(filepath);
	}

	if(!hash1 || !hash2)
		return 0;

	if(!strcmp(hash1, hash2)){
		free(hash1);
		free(hash2);
		return 1;
	}

	free(hash1);
	free(hash2);
	return 0;
}

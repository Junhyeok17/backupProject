#include "header.h"
#include <errno.h>

#define PATH 4096
#define NAME 255

int optd = 0;

void scanDirectory(char *filepath);						// 디렉토리 탐색
void copyFile(char *originfile, char *copyfile, int idx); // 백업 디렉토리에 백업
int dupCheck(char *filepath);							// 이미 백업된 파일인지 검사

int main(int argc, char *argv[])
{
	// 백업 디렉터리 경로 저장
	backupDir = (char*)malloc(strlen(getenv("HOME"))+10);
	strcpy(backupDir, getenv("HOME"));
	strcat(backupDir, "/backup");

	hashfunc = argv[argc-1];

	// 백업 디렉터리를 연결리스트에 추가하기 위한 탐색
	scanFile(backupDir);

	argc--;

	if(!(argc==2 || argc==3)){
		printf("Usage : add <FILENAME> [OPTION]\n");
		printf("  -d : add directory recursive\n\n");
		exit(0);
	}

	// 백업하려는 파일 경로 저장할 포인터 변수
	char *filename = NULL;

	// 에러 발생 시 몇 번 에러인지 저장하기 위한 변수
	int error = 0;

	// 입력 받은 경로가 절대경로일 경우 그대로 포인터 변수에 저장
	if(argv[1][0]=='/'){
		char *tmp;
		if((tmp=realpath(argv[1], NULL))){
			filename = (char*)malloc(strlen(argv[1])+1);
			strcpy(filename, argv[1]);
		}
		else
			filename = NULL;
	}
	else{
		// 입력 받은 경로가 상대경로일 경우 절대경로로 변환해서 저장하고
		filename = realpath(argv[1], NULL);
	}

	// 존재하지 않는 경로이면 에러 처리 후 프로그램 종료
	if(!filename){
		printf("Usage : add <FILENAME> [OPTION]\n");
		printf("  -d : add directory recursive\n\n");
		exit(0);
	}

	if(strlen(filename)>=PATH)	// 파일 경로 길이가 4096 이상인 경우
		error = 1;

	if(access(filename, R_OK)==-1 || access(filename, W_OK)==-1)	// 파일 접근 권한이 없는 경우
		error = 2;

	// 입력 받은 파일의 속성 불러오기
	struct stat status;
	stat(filename, &status);

	if(!(S_ISREG(status.st_mode) || S_ISDIR(status.st_mode)))
		error = 3;

	char opt;
	opterr = 0;

	optind = 0;
	// 옵션 제대로 사용했는지 검사
	while((opt=getopt(argc, argv, "d"))!=-1){	// 다른 옵션을 사용한 경우
		switch(opt){
			case 'd':
				optd = 1;
				break;
			default:
				error = 4;
				break;
		}
	}

	if(S_ISDIR(status.st_mode)){		// 디렉토리인데 d 옵션 안 쓴 경우
		if(!optd){
			error = 5;
		}
	}

	char copyfile[PATH];

	// 홈디렉터리 경로 불러오기
	strcpy(copyfile, getenv("HOME"));
	int tmp = strlen(copyfile);
	int tmp2;

	if(!strncmp(filename, copyfile, strlen(copyfile))){	// 경로가 홈 디렉토리 벗어나는지 확인
		strcat(copyfile, "/backup");
		tmp2 = strlen(copyfile);
		strcat(copyfile, filename+tmp);
	}
	else{
		printf("\"%s\" can't be backuped\n\n", filename);
		free(filename);
		exit(0);
	}

	if(!strncmp(filename, backupDir, strlen(backupDir))){	// 경로가 백업 디렉토리 포함하는지 확인
		printf("\"%s\" can't be backuped\n\n", filename);
		free(filename);
		exit(0);
	}

	// 에러 번호에 따라 에러문 출력 후 프로그램 종료
	switch(error){
		case 1:
			printf("백업할 파일 이름의 길이가 길이 제한을 넘었습니다.\n\n");
			free(filename);
			exit(0);
		case 2:
			printf("파일에 대한 접근 권한이 없습니다.\n\n");
			free(filename);
			exit(0);
		case 3:
			printf("일반 파일 또는 디렉토리가 아닙니다.\n\n");
			free(filename);
			exit(0);
		case 4:
			printf("Usage : add <FILENAME> [OPTION]\n");
			printf("  -d : add directory recursive\n\n");
			free(filename);
			exit(0);
		case 5:
			printf("\"%s\" is a directory file\n\n", filename);
			free(filename);
			exit(0);
	}

	// 백업하려는 파일 뒤에 붙일 현재 시간 불러오기
	char *backuptime = getCurrentTime();
	int idx = tmp2+2;

	int check;
	if(S_ISREG(status.st_mode)){
		// 정규 파일이면 백업 중복 체크하고 파일 백업
		check = dupCheck(filename);
		strcat(copyfile, backuptime);
		if(!check){
			copyFile(filename, copyfile, idx);
		}
		else{
			printf("\"%s\" is already backuped\n", copyfile);
		}
	}
	else if(S_ISDIR(status.st_mode) && optd){
		// 디렉터리면 d 옵션 적용 여부 확인 후 백업
		scanDirectory(filename);
	}
	exit(0);
}

void copyFile(char *originfile, char *copyfile, int idx)
{
	// 파일 백업할 때 그 이전까지의 디렉터리 경로가 존재하지 않으면 디렉터리들 생성
	while(1){
		if(copyfile[idx]=='\0')
			break;

		int fd;
		char tmppath[PATH];

		while(copyfile[idx]!='/' && copyfile[idx]!='\0')
			idx++;

		if(copyfile[idx]!='\0'){
			strncpy(tmppath, copyfile, idx);
			tmppath[idx] = '\0';

			if(!access(tmppath, F_OK)){
				idx++;
				continue;
			}
			else if((fd=mkdir(tmppath, 0777)) < 0){
				fprintf(stderr, "creat directory error for %s\n", tmppath);
				exit(0);
			}
			else{
				idx++;
			}
		}
	}

	int fd_read, fd_write;

	// 백업할 파일 생성 및 복사
	if((fd_write=open(copyfile, O_RDWR | O_CREAT, 0644)) < 0){
		fprintf(stderr, "open write %s file error\n\n", copyfile);
		perror("open file error");
		exit(1);
	}
	else{
		char buf[1024];
		int size;

		struct stat status;
		stat(originfile, &status);
		// 백업될 파일 열고 복사
		if((fd_read=open(originfile, O_RDWR)) < 0){
			fprintf(stderr, "open read %s file error\n\n", originfile);
			perror("open file error");
			exit(1);
		}
		else{
			while((size=read(fd_read, buf, 1024)) > 0){
				write(fd_write, buf, size);
			}
		}


		printf("\"%s\" backuped\n", copyfile);
	}
	close(fd_read);
	close(fd_write);
}

void scanDirectory(char *filepath)
{
	int num;
	struct dirent **list;

	// 백업할 파일이 디렉터리일 경우 내부 파일 복사
	if((num=scandir(filepath, &list, NULL, alphasort)) < 0){
		fprintf(stderr, "scandir error\n\n");
		exit(1);
	}

	for(int i=0;i<num;i++){
		if(!strcmp(list[i]->d_name, ".") || !strcmp(list[i]->d_name, "..") || !strcmp(list[i]->d_name, "backup"))
			continue;

		char *newfile = (char*)malloc(PATH);
		char *originfile = (char*)malloc(PATH);

		strcpy(originfile, filepath);
		strcat(originfile, "/");
		strcat(originfile, list[i]->d_name);

		struct stat status;
		lstat(originfile, &status);

		char *backuptime = NULL;

		if(S_ISREG(status.st_mode)){
			// 디렉터리 내의 정규파일 백업할 경우 중복 검사
			int check = dupCheck(originfile);

			int len = strlen(getenv("HOME"));
			strcpy(newfile, backupDir);
			strcat(newfile, filepath+len);
			strcat(newfile, "/");
			strcat(newfile, list[i]->d_name);

			char *backuptime = getCurrentTime();

			strcat(newfile, backuptime);

			if(!check){
				// 백업할 파일 백업 파일 관리 리스트에 추가
				addFile(newfile);
				// 그 뒤 물리적 복사
				copyFile(originfile, newfile, strlen(backupDir)+2);
			}
			else
				printf("\"%s\" is already backuped\n", newfile);
		}
		else if(S_ISDIR(status.st_mode) && optd){
			// 내부 파일이 디렉터리일 경우 같은 동작 반복
			scanDirectory(originfile);
		}
		free(backuptime);
		free(newfile);
		free(originfile);
	}
}

// 백업할 파일이 중복됐는지 검사하는 함수
int dupCheck(char *filepath)
{
	File *tmp = head;
	char *hash;

	// 해쉬 함수를 사용해서 파일 내용 해쉬 변환 후 중복 여부 검사
	if(!strcmp(hashfunc, "md5"))
		hash = hash_md5(filepath);
	else
		hash = hash_sha(filepath);

	// 파일 경로명과 해쉬 변환값이 모두 같은지 검사하여 중복 체크
	while(tmp){
		if(!strcmp(tmp->originPath, filepath) && !strcmp(tmp->hash, hash)){
			free(hash);
			return 1;
		}
		tmp = tmp->next;
	}

	free(hash);
	return 0;
}

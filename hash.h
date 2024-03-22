#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

// sha 해쉬 함수를 사용해서 파일 내용 해쉬 변환
char* hash_sha(char *filename)
{
	int i, fd;
	SHA_CTX ctx;
	unsigned char md[SHA_DIGEST_LENGTH];
	char shaResult[1024*16];
	SHA1_Init(&ctx);

	if((fd = open(filename, O_RDONLY)) < 0){
		return NULL;
	}
	while(1){
		i = read(fd, shaResult, 1024*16);
		if(i <= 0)
			break;
		SHA1_Update(&ctx, shaResult, (unsigned long)i);
	}
	SHA1_Final(&(md[0]), &ctx);

	for(int i=0;i<SHA_DIGEST_LENGTH;i++)
		sprintf(shaResult+(i*2), "%02x", md[i]);

	close(fd);
	char *result = (char*)malloc(strlen(shaResult)+1);
	strcpy(result, shaResult);
	return result;
}

// md5 해쉬 함수를 사용해서 파일 내용 해쉬 변환
char* hash_md5(char *filename)
{
	int i, fd;
	MD5_CTX ctx;
	unsigned char md[MD5_DIGEST_LENGTH];
	char mdResult[1024*16];
	MD5_Init(&ctx);

	if((fd = open(filename, O_RDONLY)) < 0){
		return NULL;
	}

	while(1){
		i = read(fd, mdResult, 1024*16);
		if(i <= 0)
			break;
		MD5_Update(&ctx, mdResult, (unsigned long)i);
	}
	MD5_Final(&(md[0]), &ctx);

	for(int i=0;i<MD5_DIGEST_LENGTH;i++)
		sprintf(mdResult+(i*2), "%02x", md[i]);

	close(fd);
	char *result = (char*)malloc(strlen(mdResult)+1);
	strcpy(result, mdResult);
	return result;
}

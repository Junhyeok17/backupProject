all : add remove recover help ssu_backup

add : add.c
	gcc -o add add.c -lcrypto

remove : remove.c
	gcc -o remove remove.c -lcrypto

recover : recover.c
	gcc -o recover recover.c -lcrypto

help : help.c
	gcc -o help help.c

ssu_backup : ssu_backup.c
	gcc -o ssu_backup ssu_backup.c -lcrypto

clean:
	rm ssu_backup
	rm add
	rm remove
	rm recover
	rm help

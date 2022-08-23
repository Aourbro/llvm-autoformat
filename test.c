#include<stdio.h>
void prt(char *buf){
	for(int i=0;i<4;i++){
		printf("%c", buf[i]);
	}
}	
	
int main(){
	char a = 2;
	int b =3;
	int c = a * b;
	char *buf = "hello";
	char *notbuf = "world";
	prt(buf);
	printf("\n");
	for(int i=0;i<4;i++){
		printf("%c", notbuf[i]);
	}
	printf("\n");
	return 0;
}

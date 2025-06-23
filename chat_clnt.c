#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 100 //메세지 버퍼 크기
#define NAME_SIZE 20 //사용자 이름 버퍼 크기
	
void * send_msg(void * arg); //메세지를 서버로 전송할 스레드 함수
void * recv_msg(void * arg); //서버로부터 메세지를 수신할 스레드 함수
void error_handling(char * msg); //오류 발생 시 메세지를 출력하고 종료하는 함수
	
char name[NAME_SIZE]="[DEFAULT]"; //클라이언트 이름을 저장할 배열
char msg[BUF_SIZE]; //사용자 입력 메세지를 저장할 버퍼
	
int main(int argc, char *argv[])
{
	int sock; //클라이언트 소켓 변수
	struct sockaddr_in serv_addr; //서버 주소 정보 구조체
	pthread_t snd_thread, rcv_thread; //메세지 송신&수신을 저장할 변수
	void * thread_return; //스레드 종료 시 반환값을 받을 변수
	if(argc!=4) { //프로그램 실행 시 IP, Port#, 이름이 인자로 주어지지 않은 경우
		printf("Usage : %s <IP> <port> <name>\n", argv[0]); //사용법 출력
		exit(1); //프로그램 종료
	 }
	
	sprintf(name, "[%s]", argv[3]); //명령줄 인자로 받은 이름을 name 변수에 저장
	//1. 소켓 생성
	sock=socket(PF_INET, SOCK_STREAM, 0); //소켓 생성
	
	//2. 서버 주소 정보 초기화 및 설정
	memset(&serv_addr, 0, sizeof(serv_addr)); //서버 주소 구조체 초기화
	serv_addr.sin_family=AF_INET; //IPv4
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]); //서버 IP 주소로 문자열을 네트워크 주소로 변환하여 설정
	serv_addr.sin_port=htons(atoi(argv[2])); //서버 Port#으로 문자열을 정수로 변환하여 설정
	  
	//3. connect(서버에 연결 요청)
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) //서버에 연결 요청
		error_handling("connect() error"); //연결 실패 시 오류 처리
	
	//4. 송수신 처리를 위한 스레드 각각 생성
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);// 메시지 송신 스레드 생성으로 send_msg 함수가 클라이언트 소켓을 인자로 받아 실행됨
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);// 메시지 수신 스레드 생성으로 recv_msg 함수가 클라이언트 소켓을 인자로 받아 실행됨
	
	//5. 생성된 스레드들이 종료될 때까지 대기
	pthread_join(snd_thread, &thread_return); //송신 스레드가 종료될 때까지 대기
	pthread_join(rcv_thread, &thread_return); //수신 스레드가 종료될 때까지 대기
	close(sock); //클라이언트 소켓 닫기
	return 0; //메인 함수 종료
}
	
void * send_msg(void * arg) //메세지 송신 스레드 함수
{
	int sock=*((int*)arg); //스레드 생성 시 전달받은 클라이언트 소켓
	char name_msg[NAME_SIZE+BUF_SIZE]; //이름과 메세지를 합쳐 저장할 버퍼
	while(1) //사용자 입력 계속 대기
	{
		fgets(msg, BUF_SIZE, stdin); //키보드로부터 메세지 읽어서 저장
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) //입력된 메세지가 "q" 또는 "Q"이면
		{
			close(sock); //소켓 닫기
			exit(0); //프로그램 종료
		}
		sprintf(name_msg,"%s %s", name, msg); //이름과 입력 메세지를 합쳐 name_msg에 저장
		write(sock, name_msg, strlen(name_msg)); //조합된 메세지를 서버로 전송
	}
	return NULL; // 스레드 종료
}
	
void * recv_msg(void * arg) //메세지 수신 스레드 함수
{
	int sock=*((int*)arg); //스레드 생성 시 전달받은 클라이언트 소켓
	char name_msg[NAME_SIZE+BUF_SIZE]; //서버로부터 받은 메세지를 저장할 버퍼
	int str_len; //읽어들인 메세지 길이
	while(1) //서버로부터 메세지 계속 수신 대기
	{
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1); //서버로부터 메세지 읽기
		if(str_len==-1) //read 함수가 -1을 반환하면 오류 발생 또는 서버 연결 끊김
			return (void*)-1; //-1을 반환하며 오륧 스레드 종료
		name_msg[str_len]=0; //읽어들인 데이터의 끝에 널 문자 추가
		fputs(name_msg, stdout); //수신한 메세지를 출력
	}
	return NULL; //스레드 종료
}
	
void error_handling(char *msg) //오류 처리 함수
{
	fputs(msg, stderr); //오류 메세지 출력
	fputc('\n', stderr);
	exit(1); //프로그램 종료
}

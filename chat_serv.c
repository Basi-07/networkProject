#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100 //메세지 버퍼 크기
#define MAX_CLNT 256 //최대 동시 접속 클라이언트 수

void * handle_clnt(void * arg); //클라이언트 요청 처리 함수
void send_msg(char * msg, int len); //클라이언트 메세지 전송
void error_handling(char * msg); //오류 발생 시 메세지 출력&종료 함수

int clnt_cnt=0; //현재 접속된 클라이언트 수를 저장하는 변수
int clnt_socks[MAX_CLNT]; //연결된 클라이언트들의 소켓 디스크립터 배열
pthread_mutex_t mutx; //멀티스레드 환경에서 자원보호를 위한 뮤텍스

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock; //서버, 클라이언트 소켓 fd
	struct sockaddr_in serv_adr, clnt_adr; //서버& 클라이언트 주소 정보 구조체
	int clnt_adr_sz; //클라이언트 주소 구조체 크기
	pthread_t t_id; //새로 생성될 스레드 ID
	if(argc!=2) { //프로그램 실행 시 Port#를 받지 않았을 경우
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
  
	pthread_mutex_init(&mutx, NULL); //clnt_cnt와 clnt_socks의 동시 접근을 막기 위한 뮤텍스 초기화
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); //TCP 소켓 생성

	memset(&serv_adr, 0, sizeof(serv_adr)); //서버 주소 초기화
	serv_adr.sin_family=AF_INET; //IPv4
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //모든 인터페이스 접속 허용
 	serv_adr.sin_port=htons(atoi(argv[1])); //서버 Port#
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1) //소켓 주소 정보 할당
		error_handling("bind() error"); //바인딩 실패시 오류 처리
	if(listen(serv_sock, 5)==-1) //클라이언트 연결 요청 대기
		error_handling("listen() error"); //리슨 실패시 오류 처리
	
	while(1) //클라이언트 연결 계속 대기
	{
		clnt_adr_sz=sizeof(clnt_adr); //클라이언트 주소 구조체 크기 설정
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz); //클라이언트 연결 요청 수락
		
		pthread_mutex_lock(&mutx); //clnt_socks 배열 접근 전 뮤텍스 잠금
		clnt_socks[clnt_cnt++]=clnt_sock; //새로 연결된 클라이언트 소켓을 배열에 추가 후 클라이언트 수 증가
		pthread_mutex_unlock(&mutx); //clnt_socks 배열 접근 후 뮤텍스 잠금 해제
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); //새로운 스레드가 형성되어 함수가 클라이언트 소켓을 인자로 받아 실행됨
		pthread_detach(t_id); //메모리 누수를 막기 위해 스레드를 메인 스레드로 부터 분리하여 스레드 종료 시 자동으로 해제
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr)); //연결된 클라이언트 IP 주소 출력
	}
	close(serv_sock); //서버 소켓 닫음
	return 0; //함수 종료
}
	
void* handle_clnt(void* arg) //클라이언트 요청 처리 스레드 함수
{
	int clnt_sock = *((int*)arg); //스레드 생성 시 전달받은 소켓 디스크립터
	int str_len = 0, i; //메세지 길이
	char msg[BUF_SIZE]; //클라이언트로부터 받은 메세지를 저장할 버퍼

	while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) //클라이언트로부터 메세지 읽음
		send_msg(msg, str_len); //읽은 메세지를 모든 클라이언트에게 전송

	pthread_mutex_lock(&mutx); //클라이언트 연결이 끊어졌을 때 clnt_socks 배열 접근 전 뮤텍스 잠금
	for (i = 0; i < clnt_cnt; i++) //현재 연결된 클라이언트 수만큼 반복
	{
		if (clnt_sock == clnt_socks[i]) //연결이 끊긴 클라이언트의 소켓을 배열에서 찾으면
		{
			while (i < clnt_cnt - 1) //해당 소켓을 제거하고 배열 뒤의 요소들을 한 칸씩 앞으로 당김
			{
				clnt_socks[i] = clnt_socks[i + 1];
				i++; //다음 요소로 이동
			}
			break; //해당 소켓을 찾고 제거했으면 종료
		}
	}
	clnt_cnt--; //전체 클라이언트 수 감소
	pthread_mutex_unlock(&mutx); //clnt_socks 배열 접근 후 뮤텍스 잠금 해제
	close(clnt_sock); //해당 클라이언트 소켓 닫기
	return NULL; //스레드 종료
}

void send_msg(char* msg, int len) // 모든 클라이언트에게 메시지를 전송하는 함수
{
	int i;
	pthread_mutex_lock(&mutx); //clnt_socks 배열 접근 전 뮤텍스 잠금
	for (i = 0; i < clnt_cnt; i++) //현재 연결된 모든 클라이언트에게
		write(clnt_socks[i], msg, len); //메세지 전송
	pthread_mutex_unlock(&mutx); //clnt_socks 배열 접근 후 뮤텍스 잠금 해제
}

void error_handling(char* msg) //오류 처리 함수
{
	fputs(msg, stderr); //오류 메세지 출력
	fputc('\n', stderr);
	exit(1); //프로그램 종료
}
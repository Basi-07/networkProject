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
#define NAME_SIZE 20 //이름 크기 정의

void * handle_clnt(void * arg); //클라이언트 요청 처리 함수
void send_msg(char * msg, int len); //클라이언트 메세지 전송
void error_handling(char * msg); //오류 발생 시 메세지 출력&종료 함수

int clnt_cnt=0; //현재 접속된 클라이언트 수를 저장하는 변수
int clnt_socks[MAX_CLNT]; //연결된 클라이언트들의 소켓 디스크립터 배열
char clnt_names[MAX_CLNT][NAME_SIZE] = {0,}; //이름 저장 배열
pthread_mutex_t mutx; //멀티스레드 환경에서 자원보호를 위한 뮤텍스

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock; //서버, 클라이언트 소켓 fd
	struct sockaddr_in serv_adr, clnt_adr; //서버& 클라이언트 주소 정보 구조체
	int clnt_adr_sz; //클라이언트 주소 구조체 크기
	pthread_t t_id; //새로 생성될 스레드 ID
	if(argc!=2) { //프로그램 실행 시 Port#를 받지 않았을 경우
		printf("Usage : %s <port>\n", argv[0]); //사용자에게 올바른 사용법 알려주기 위한 안내메세지 보여줌
		exit(1); //프로그램 종료
	}
  
	pthread_mutex_init(&mutx, NULL); //clnt_cnt와 clnt_socks의 동시 접근을 막기 위한 뮤텍스 초기화
	//1. 소켓 생성
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); //TCP 소켓 생성

	//2. 서버 주소 정보 초기화 및 설정
	memset(&serv_adr, 0, sizeof(serv_adr)); //서버 주소 초기화
	serv_adr.sin_family=AF_INET; //IPv4
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //모든 인터페이스 접속 허용,IP주소를 자동으로 할당
 	serv_adr.sin_port=htons(atoi(argv[1])); //서버 Port#
	
	//3. bind(소켓에 주소 정보 할당)
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1) //소켓 주소 정보 할당
		error_handling("bind() error"); //바인딩 실패시 오류 처리
	
	//4. listen(연결 요청 대기 상태)
	if(listen(serv_sock, 5)==-1) //클라이언트 연결 요청 대기
		error_handling("listen() error"); //리슨 실패시 오류 처리
	
	//5. accept(클라이언트 연결 요청 수락)
	while(1) //클라이언트 연결 계속 대기
	{
		clnt_adr_sz=sizeof(clnt_adr); //클라이언트 주소 구조체 크기 설정
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz); //클라이언트 연결 요청 수락

		//임계 구역 시작
		//새로운 클라이언트 정보를 전역 변수에 등록 
		pthread_mutex_lock(&mutx); //clnt_socks 배열 접근 전 뮤텍스 잠금
        clnt_list[clnt_cnt].sock = clnt_sock; //클라이언트 소켓 번호를 배열의 현재 위치에 저장
		pthread_mutex_unlock(&mutx); //clnt_socks 배열 접근 후 뮤텍스 잠금 해제
		
		//6. 클라이언트 처리를 위한 스레드 생성
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); //새로운 스레드가 형성되어 함수가 클라이언트 소켓을 인자로 받아 실행됨
		
		//종료되길 기다릴 필요 없이 즉시 다음 클라이언트 받을 수 있음
		pthread_detach(t_id); //메모리 누수를 막기 위해 스레드를 메인 스레드로 부터 분리하여 스레드 종료 시 자동으로 해제
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr)); //연결된 클라이언트 IP 주소 출력
	}
	close(serv_sock); //서버 소켓 닫음
	return 0; //함수 종료
}

//클라이언트 1명당 1개씩 생성
void* handle_clnt(void* arg) //클라이언트 요청 처리 스레드 함수
{	
	int clnt_sock = *((int*)arg); //스레드 생성 시 전달받은 소켓 디스크립터
	int str_len = 0, i; //메세지 길이
	char msg[BUF_SIZE]; //클라이언트로부터 받은 메세지를 저장할 버퍼
	int my_idx = -1; // 현재 클라이언트의 배열 인덱스

    //현재 클라이언트의 인덱스 찾기
    pthread_mutex_lock(&mutx); //배열 접근을 위해 뮤텍스 잠금
    for (i = 0; i < clnt_cnt; i++) {
        if (clnt_sock == clnt_socks[i]) { //소켓이 배열에 있는지 확인
            my_idx = i; //자신의 인덱스 찾음
            break;
        }
    }
    pthread_mutex_unlock(&mutx); //뮤텍스 해제

    while ((str_len = read(clnt_sock, msg, sizeof(msg)-1)) > 0) { //메세지를 계속 읽음
        msg[str_len] = 0; //종료 문자 추가

        //클라이언트의 이름이 아직 등록되지 않았다면
        if (strlen(clnt_names[my_idx]) == 0) {
            char* start = strchr(msg, '['); //메세지에서 이름을 추출하여 등록
            char* end = strchr(msg, ']');
            if (start != NULL && end != NULL && (end > start)) { //이름 형식 맞으면 배열에 저장
                strncpy(clnt_names[my_idx], start + 1, end - start - 1); //이름 안의 문자열 복사
                //strncpy는 NULL 문자를 보장하지 않으므로 직접 추가
                clnt_names[my_idx][end - start - 1] = '\0'; //문자열 끝에 직접 NULL추가
            }
        }
        
        char* whisper_ptr = strstr(msg, "@"); // 메세지 내용에서 @를 찾아 귓속말인지 판별
        char* name_end_bracket = strchr(msg, ']'); // 이름 끝남
		//귓속말 형식
        if (whisper_ptr != NULL && name_end_bracket != NULL && whisper_ptr == (name_end_bracket + 2)) {
            char to_name[NAME_SIZE] = {0,}; //받을 사람 이름 저장 변수
            char notice_msg[BUF_SIZE]; //알림 메세지 버퍼
            int target_idx = -1; //대상 클라이언트 인덱스

            sscanf(whisper_ptr, "@%s", to_name); //받는 사람 이름 파싱

            pthread_mutex_lock(&mutx); //받는 사람의 인덱스 찾기
            for (i = 0; i < clnt_cnt; i++) {
                if (strcmp(to_name, clnt_names[i]) == 0) { //이름 비교
                    target_idx = i;
                    break;
                }
            }
            pthread_mutex_unlock(&mutx); //잠금 해제

            if (target_idx != -1) { //귓속말 전송
                write(clnt_socks[target_idx], msg, strlen(msg)); //받는 사람에게 전송
                write(clnt_sock, msg, strlen(msg)); //보낸 사람에게도 전송
            } else {
                //사용자가 없는 경우, 보낸 사람에게만 알림
                sprintf(notice_msg, " %s를 찾을 수 없습니다. ##\n", to_name);
                write(clnt_sock, notice_msg, strlen(notice_msg));
            }
        }
        else { //귓속말이 아닌 경우 그냥 전송
            send_msg(msg, strlen(msg));
        }
    }

    //연결 종료 처리
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i]) {
            //나가는 클라이언트의 이름 정보를 지우고 배열을 한 칸씩 당김
            while (i < clnt_cnt - 1) {
                clnt_socks[i] = clnt_socks[i + 1]; //소켓이동
                strcpy(clnt_names[i], clnt_names[i+1]); //이름이동
                i++;
            }
            break; //찾으면 종료
        }
    }
	clnt_cnt--; //전체 클라이언트 수 감소
	pthread_mutex_unlock(&mutx); //clnt_socks 배열 접근 후 뮤텍스 잠금 해제
	close(clnt_sock); //해당 클라이언트 소켓 닫기
	return NULL; //스레드 종료
}

void send_msg(char* msg, int len) //모든 클라이언트에게 메시지를 전송하는 함수
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
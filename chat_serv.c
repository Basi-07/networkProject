#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100 //�޼��� ���� ũ��
#define MAX_CLNT 256 //�ִ� ���� ���� Ŭ���̾�Ʈ ��

void * handle_clnt(void * arg); //Ŭ���̾�Ʈ ��û ó�� �Լ�
void send_msg(char * msg, int len); //Ŭ���̾�Ʈ �޼��� ����
void error_handling(char * msg); //���� �߻� �� �޼��� ���&���� �Լ�

int clnt_cnt=0; //���� ���ӵ� Ŭ���̾�Ʈ ���� �����ϴ� ����
int clnt_socks[MAX_CLNT]; //����� Ŭ���̾�Ʈ���� ���� ��ũ���� �迭
pthread_mutex_t mutx; //��Ƽ������ ȯ�濡�� �ڿ���ȣ�� ���� ���ؽ�

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock; //����, Ŭ���̾�Ʈ ���� fd
	struct sockaddr_in serv_adr, clnt_adr; //����& Ŭ���̾�Ʈ �ּ� ���� ����ü
	int clnt_adr_sz; //Ŭ���̾�Ʈ �ּ� ����ü ũ��
	pthread_t t_id; //���� ������ ������ ID
	if(argc!=2) { //���α׷� ���� �� Port#�� ���� �ʾ��� ���
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
  
	pthread_mutex_init(&mutx, NULL); //clnt_cnt�� clnt_socks�� ���� ������ ���� ���� ���ؽ� �ʱ�ȭ
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); //TCP ���� ����

	memset(&serv_adr, 0, sizeof(serv_adr)); //���� �ּ� �ʱ�ȭ
	serv_adr.sin_family=AF_INET; //IPv4
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //��� �������̽� ���� ���
 	serv_adr.sin_port=htons(atoi(argv[1])); //���� Port#
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1) //���� �ּ� ���� �Ҵ�
		error_handling("bind() error"); //���ε� ���н� ���� ó��
	if(listen(serv_sock, 5)==-1) //Ŭ���̾�Ʈ ���� ��û ���
		error_handling("listen() error"); //���� ���н� ���� ó��
	
	while(1) //Ŭ���̾�Ʈ ���� ��� ���
	{
		clnt_adr_sz=sizeof(clnt_adr); //Ŭ���̾�Ʈ �ּ� ����ü ũ�� ����
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz); //Ŭ���̾�Ʈ ���� ��û ����
		
		pthread_mutex_lock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ���
		clnt_socks[clnt_cnt++]=clnt_sock; //���� ����� Ŭ���̾�Ʈ ������ �迭�� �߰� �� Ŭ���̾�Ʈ �� ����
		pthread_mutex_unlock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ��� ����
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); //���ο� �����尡 �����Ǿ� �Լ��� Ŭ���̾�Ʈ ������ ���ڷ� �޾� �����
		pthread_detach(t_id); //�޸� ������ ���� ���� �����带 ���� ������� ���� �и��Ͽ� ������ ���� �� �ڵ����� ����
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr)); //����� Ŭ���̾�Ʈ IP �ּ� ���
	}
	close(serv_sock); //���� ���� ����
	return 0; //�Լ� ����
}
	
void* handle_clnt(void* arg) //Ŭ���̾�Ʈ ��û ó�� ������ �Լ�
{
	int clnt_sock = *((int*)arg); //������ ���� �� ���޹��� ���� ��ũ����
	int str_len = 0, i; //�޼��� ����
	char msg[BUF_SIZE]; //Ŭ���̾�Ʈ�κ��� ���� �޼����� ������ ����

	while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) //Ŭ���̾�Ʈ�κ��� �޼��� ����
		send_msg(msg, str_len); //���� �޼����� ��� Ŭ���̾�Ʈ���� ����

	pthread_mutex_lock(&mutx); //Ŭ���̾�Ʈ ������ �������� �� clnt_socks �迭 ���� �� ���ؽ� ���
	for (i = 0; i < clnt_cnt; i++) //���� ����� Ŭ���̾�Ʈ ����ŭ �ݺ�
	{
		if (clnt_sock == clnt_socks[i]) //������ ���� Ŭ���̾�Ʈ�� ������ �迭���� ã����
		{
			while (i < clnt_cnt - 1) //�ش� ������ �����ϰ� �迭 ���� ��ҵ��� �� ĭ�� ������ ���
			{
				clnt_socks[i] = clnt_socks[i + 1];
				i++; //���� ��ҷ� �̵�
			}
			break; //�ش� ������ ã�� ���������� ����
		}
	}
	clnt_cnt--; //��ü Ŭ���̾�Ʈ �� ����
	pthread_mutex_unlock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ��� ����
	close(clnt_sock); //�ش� Ŭ���̾�Ʈ ���� �ݱ�
	return NULL; //������ ����
}

void send_msg(char* msg, int len) // ��� Ŭ���̾�Ʈ���� �޽����� �����ϴ� �Լ�
{
	int i;
	pthread_mutex_lock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ���
	for (i = 0; i < clnt_cnt; i++) //���� ����� ��� Ŭ���̾�Ʈ����
		write(clnt_socks[i], msg, len); //�޼��� ����
	pthread_mutex_unlock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ��� ����
}

void error_handling(char* msg) //���� ó�� �Լ�
{
	fputs(msg, stderr); //���� �޼��� ���
	fputc('\n', stderr);
	exit(1); //���α׷� ����
}
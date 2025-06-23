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
#define NAME_SIZE 20 //�̸� ũ�� ����

void * handle_clnt(void * arg); //Ŭ���̾�Ʈ ��û ó�� �Լ�
void send_msg(char * msg, int len); //Ŭ���̾�Ʈ �޼��� ����
void error_handling(char * msg); //���� �߻� �� �޼��� ���&���� �Լ�

int clnt_cnt=0; //���� ���ӵ� Ŭ���̾�Ʈ ���� �����ϴ� ����
int clnt_socks[MAX_CLNT]; //����� Ŭ���̾�Ʈ���� ���� ��ũ���� �迭
char clnt_names[MAX_CLNT][NAME_SIZE] = {0,}; //�̸� ���� �迭
pthread_mutex_t mutx; //��Ƽ������ ȯ�濡�� �ڿ���ȣ�� ���� ���ؽ�

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock; //����, Ŭ���̾�Ʈ ���� fd
	struct sockaddr_in serv_adr, clnt_adr; //����& Ŭ���̾�Ʈ �ּ� ���� ����ü
	int clnt_adr_sz; //Ŭ���̾�Ʈ �ּ� ����ü ũ��
	pthread_t t_id; //���� ������ ������ ID
	if(argc!=2) { //���α׷� ���� �� Port#�� ���� �ʾ��� ���
		printf("Usage : %s <port>\n", argv[0]); //����ڿ��� �ùٸ� ���� �˷��ֱ� ���� �ȳ��޼��� ������
		exit(1); //���α׷� ����
	}
  
	pthread_mutex_init(&mutx, NULL); //clnt_cnt�� clnt_socks�� ���� ������ ���� ���� ���ؽ� �ʱ�ȭ
	//1. ���� ����
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); //TCP ���� ����

	//2. ���� �ּ� ���� �ʱ�ȭ �� ����
	memset(&serv_adr, 0, sizeof(serv_adr)); //���� �ּ� �ʱ�ȭ
	serv_adr.sin_family=AF_INET; //IPv4
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //��� �������̽� ���� ���,IP�ּҸ� �ڵ����� �Ҵ�
 	serv_adr.sin_port=htons(atoi(argv[1])); //���� Port#
	
	//3. bind(���Ͽ� �ּ� ���� �Ҵ�)
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1) //���� �ּ� ���� �Ҵ�
		error_handling("bind() error"); //���ε� ���н� ���� ó��
	
	//4. listen(���� ��û ��� ����)
	if(listen(serv_sock, 5)==-1) //Ŭ���̾�Ʈ ���� ��û ���
		error_handling("listen() error"); //���� ���н� ���� ó��
	
	//5. accept(Ŭ���̾�Ʈ ���� ��û ����)
	while(1) //Ŭ���̾�Ʈ ���� ��� ���
	{
		clnt_adr_sz=sizeof(clnt_adr); //Ŭ���̾�Ʈ �ּ� ����ü ũ�� ����
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz); //Ŭ���̾�Ʈ ���� ��û ����

		//�Ӱ� ���� ����
		//���ο� Ŭ���̾�Ʈ ������ ���� ������ ��� 
		pthread_mutex_lock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ���
        clnt_list[clnt_cnt].sock = clnt_sock; //Ŭ���̾�Ʈ ���� ��ȣ�� �迭�� ���� ��ġ�� ����
		pthread_mutex_unlock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ��� ����
		
		//6. Ŭ���̾�Ʈ ó���� ���� ������ ����
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); //���ο� �����尡 �����Ǿ� �Լ��� Ŭ���̾�Ʈ ������ ���ڷ� �޾� �����
		
		//����Ǳ� ��ٸ� �ʿ� ���� ��� ���� Ŭ���̾�Ʈ ���� �� ����
		pthread_detach(t_id); //�޸� ������ ���� ���� �����带 ���� ������� ���� �и��Ͽ� ������ ���� �� �ڵ����� ����
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr)); //����� Ŭ���̾�Ʈ IP �ּ� ���
	}
	close(serv_sock); //���� ���� ����
	return 0; //�Լ� ����
}

//Ŭ���̾�Ʈ 1��� 1���� ����
void* handle_clnt(void* arg) //Ŭ���̾�Ʈ ��û ó�� ������ �Լ�
{	
	int clnt_sock = *((int*)arg); //������ ���� �� ���޹��� ���� ��ũ����
	int str_len = 0, i; //�޼��� ����
	char msg[BUF_SIZE]; //Ŭ���̾�Ʈ�κ��� ���� �޼����� ������ ����
	int my_idx = -1; // ���� Ŭ���̾�Ʈ�� �迭 �ε���

    //���� Ŭ���̾�Ʈ�� �ε��� ã��
    pthread_mutex_lock(&mutx); //�迭 ������ ���� ���ؽ� ���
    for (i = 0; i < clnt_cnt; i++) {
        if (clnt_sock == clnt_socks[i]) { //������ �迭�� �ִ��� Ȯ��
            my_idx = i; //�ڽ��� �ε��� ã��
            break;
        }
    }
    pthread_mutex_unlock(&mutx); //���ؽ� ����

    while ((str_len = read(clnt_sock, msg, sizeof(msg)-1)) > 0) { //�޼����� ��� ����
        msg[str_len] = 0; //���� ���� �߰�

        //Ŭ���̾�Ʈ�� �̸��� ���� ��ϵ��� �ʾҴٸ�
        if (strlen(clnt_names[my_idx]) == 0) {
            char* start = strchr(msg, '['); //�޼������� �̸��� �����Ͽ� ���
            char* end = strchr(msg, ']');
            if (start != NULL && end != NULL && (end > start)) { //�̸� ���� ������ �迭�� ����
                strncpy(clnt_names[my_idx], start + 1, end - start - 1); //�̸� ���� ���ڿ� ����
                //strncpy�� NULL ���ڸ� �������� �����Ƿ� ���� �߰�
                clnt_names[my_idx][end - start - 1] = '\0'; //���ڿ� ���� ���� NULL�߰�
            }
        }
        
        char* whisper_ptr = strstr(msg, "@"); // �޼��� ���뿡�� @�� ã�� �ӼӸ����� �Ǻ�
        char* name_end_bracket = strchr(msg, ']'); // �̸� ����
		//�ӼӸ� ����
        if (whisper_ptr != NULL && name_end_bracket != NULL && whisper_ptr == (name_end_bracket + 2)) {
            char to_name[NAME_SIZE] = {0,}; //���� ��� �̸� ���� ����
            char notice_msg[BUF_SIZE]; //�˸� �޼��� ����
            int target_idx = -1; //��� Ŭ���̾�Ʈ �ε���

            sscanf(whisper_ptr, "@%s", to_name); //�޴� ��� �̸� �Ľ�

            pthread_mutex_lock(&mutx); //�޴� ����� �ε��� ã��
            for (i = 0; i < clnt_cnt; i++) {
                if (strcmp(to_name, clnt_names[i]) == 0) { //�̸� ��
                    target_idx = i;
                    break;
                }
            }
            pthread_mutex_unlock(&mutx); //��� ����

            if (target_idx != -1) { //�ӼӸ� ����
                write(clnt_socks[target_idx], msg, strlen(msg)); //�޴� ������� ����
                write(clnt_sock, msg, strlen(msg)); //���� ������Ե� ����
            } else {
                //����ڰ� ���� ���, ���� ������Ը� �˸�
                sprintf(notice_msg, " %s�� ã�� �� �����ϴ�. ##\n", to_name);
                write(clnt_sock, notice_msg, strlen(notice_msg));
            }
        }
        else { //�ӼӸ��� �ƴ� ��� �׳� ����
            send_msg(msg, strlen(msg));
        }
    }

    //���� ���� ó��
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i]) {
            //������ Ŭ���̾�Ʈ�� �̸� ������ ����� �迭�� �� ĭ�� ���
            while (i < clnt_cnt - 1) {
                clnt_socks[i] = clnt_socks[i + 1]; //�����̵�
                strcpy(clnt_names[i], clnt_names[i+1]); //�̸��̵�
                i++;
            }
            break; //ã���� ����
        }
    }
	clnt_cnt--; //��ü Ŭ���̾�Ʈ �� ����
	pthread_mutex_unlock(&mutx); //clnt_socks �迭 ���� �� ���ؽ� ��� ����
	close(clnt_sock); //�ش� Ŭ���̾�Ʈ ���� �ݱ�
	return NULL; //������ ����
}

void send_msg(char* msg, int len) //��� Ŭ���̾�Ʈ���� �޽����� �����ϴ� �Լ�
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
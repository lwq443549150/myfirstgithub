
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <unistd.h>
#include <semaphore.h>  //信号量
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define NSECTOSEC    1000000000

pthread_t pth[2];
sem_t sem;
int semValue;


/**网络变量通信*/
int socket_descriptor; //套接口描述字

/**************************************************/
/*
*/
int setsem_wait(int nano_sec,int sec)
{
    struct timespec ts;
    if ( clock_gettime( CLOCK_REALTIME,&ts ) < 0 )
        return -1;
    ts.tv_sec  += sec;
    ts.tv_nsec += nano_sec;
    ts.tv_sec += ts.tv_nsec/NSECTOSEC; //Nanoseconds [0 .. 999999999]
    ts.tv_nsec = ts.tv_nsec%NSECTOSEC;
    return sem_timedwait( &sem,&ts );
}

//udp 初始化
void udp_communication_init(void)
{

}
/**/
void *thread1(void *argv)
{
      while(1)
    {
       sem_post(&sem);
        sleep(3);
     //  printf("thread1\n");
    }
}

void *thread2(void *argv)
{
    while(1)
    {
       if(setsem_wait(0,1) == 0)
       {
         sem_getvalue(&sem, &semValue);
         printf("rev sem\n");
       }
       else
       {
            printf("sem time out\n");
       }
        usleep(100);
    }
}

//*******************main****************
/*
int main(void)
{
    void *returnValue;

    sem_init(&sem,0,0);
    pthread_create(&pth[0], NULL, &thread1, &pth[0]);
    pthread_create(&pth[1], NULL, &thread2, &pth[1]);


    pthread_join(pth[0], &returnValue);
    pthread_join(pth[1], &returnValue);
    return 0;
}



#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int port=8080;
int main(int argc, char** argv)
{
    int socket_descriptor; //套接口描述字
    int iter=0;
    char buf[80];
    struct sockaddr_in address;//处理网络通信的地址

    bzero(&address,sizeof(address));
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=inet_addr("192.168.43.253");//这里不一样
    address.sin_port=htons(port);    //创建一个 UDP socket

    socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);//IPV4  SOCK_DGRAM 数据报套接字（UDP协议）

    for(iter=0;iter<=20;iter++)
    {
        sprintf(buf,"data packet with ID %d\n",iter);

        /*int PASCAL FAR sendto( SOCKET s, const char FAR* buf, int len, int flags,const struct sockaddr FAR* to, int tolen);　　
         * s：一个标识套接口的描述字。　
         * buf：包含待发送数据的缓冲区。　　
         * len：buf缓冲区中数据的长度。　
         * flags：调用方式标志位。　　
         * to：（可选）指针，指向目的套接口的地址。　
         * tolen：to所指地址的长度。
　　      *
        sendto(socket_descriptor,buf,sizeof(buf),0,(struct sockaddr *)&address,sizeof(address));
    }

    sprintf(buf,"stop\n");
    sendto(socket_descriptor,buf,sizeof(buf),0,(struct sockaddr *)&address,sizeof(address));//发送stop 命令
    close(socket_descriptor);

    exit(0);

    return (EXIT_SUCCESS);
}*/

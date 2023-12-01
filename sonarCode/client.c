/*
 * @Author: oyw 1622945822@qq.com
 * @Date: 2023-11-21 14:33:10
 * @LastEditors: oyw 1622945822@qq.com
 * @LastEditTime: 2023-11-30 16:41:10
 * @FilePath: \sonarCode\client.c
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "table.h"
#include "process.h"

struct _dataBuffer dataBuffer = {NULL,0,0,NULL,0,0};
struct _OsBufferEntry OsBufferEntry = { 0, 0, 0 ,NULL,NULL,0,NULL,0};

int main()
{
    // //���ͻ�����
    // int64_t nSendBuf=32*1024;//����Ϊ32K
    // setsockopt(s,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
    // SetCtrl(lowFreMode);

    int clientFd = socket(AF_INET,SOCK_STREAM,0);
    if(clientFd ==-1){
        printf("create socket error");
        return -1;
    }
    /* ���ý��ջ����� */ 
    //int64_t nRecvBuf=200000;
    int nRecvBuf = 200000; //
    // int bytesAvailable = 0 ; 
    // socklen_t len = sizeof(bytesAvailable);
    if(setsockopt(clientFd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(nRecvBuf)) == -1){
        printf("set recv buffer err\n");
        return -1;
    }
    // }else {
    //     if( getsockopt(clientFd, SOL_SOCKET, SO_RCVBUF, &bytesAvailable, &len) ==-1 ) 
    //         printf("recv err\n");
    //     else
    //         printf("recv buffer size : %ld \n",bytesAvailable);
    // }
    /* connect server */
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("192.168.1.90"); /* ���ɵ�ip��ַ */
    serveraddr.sin_port = htons(52100); /* 52100 --> ��ǰ������  52102 --> �����˿ڣ�udp��  */

    if(connect(clientFd,(struct sockaddr*)&serveraddr,sizeof(serveraddr))){
        printf("connect error\n");
        return -1;
    } else {
        printf("connect success\n");
        FireSonar(lowFreMode); /* ��Ƶ�� ��ˮ */
        printf("Fire success\n");
    }
    
    while(1){
        unsigned nSend = 0; /* ���͵��׽��ֵ��е��ֽ� */
        if(dataBuffer.m_pToSend && (dataBuffer.m_nToSend > 0 )){
            /* send data */
            nSend++;
            nSend = send(clientFd,dataBuffer.m_pToSend,dataBuffer.m_nToSend,0);
            printf("nSend = %d\n");
            free(dataBuffer.m_pToSend);
            dataBuffer.m_pToSend = NULL;
            dataBuffer.m_nToSend = 0 ;
        }
        /* receive data */
        /* �����׽��ֵĻ�����ʣ��ռ� */
        int64_t bytesAvailable = 0;
        socklen_t len = sizeof(bytesAvailable);
        if(getsockopt(clientFd, SOL_SOCKET, SO_RCVBUF, &bytesAvailable, &len) == -1){
            printf("recv recvBUffer err\n");
            return -1;
        } else {
            printf("recv buffer size : %d \n",bytesAvailable);
        }
        if(bytesAvailable > 0){

            if((dataBuffer.m_nRxIn + bytesAvailable) > dataBuffer.m_nRxMax){
                dataBuffer.m_nRxMax = dataBuffer.m_nRxIn + bytesAvailable;
                dataBuffer.m_pRxBuffer = (char*)realloc(dataBuffer.m_pRxBuffer,dataBuffer.m_nRxMax);  /* dataBuffer.m_pRxBuffer ��ʱ�����ڴ� */
            }
            unsigned bytesRead = recv(clientFd,dataBuffer.m_pRxBuffer[dataBuffer.m_nRxIn],bytesAvailable,0);
            printf("bytesRead = %d\n",bytesRead);
            dataBuffer.m_nRxIn += bytesRead;
            /* ������ܵ������� */
            ProcessRxBuffer();
        }
        /* close socket */
        
    }
    close(clientFd); /* ʲôʱ��رգ�*/
}
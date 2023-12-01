/*
 * @Author: oyw 1622945822@qq.com
 * @Date: 2023-11-24 14:32:21
 * @LastEditors: oyw 1622945822@qq.com
 * @LastEditTime: 2023-11-30 19:44:54
 * @FilePath: \sonarCode\process.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "process.h"

/* mode 为发生频率的快慢 */
void FireSonar(int workFrequence)
{
    /* 写入声纳默认设置参数 */
    switch(WATERENVI){ /* 默认在淡水执行任务 */
        case FREASH :
            Fire(workFrequence, 10.0, 50.0, 0.0, 0.0, 1, 150, 0xff);
            break;
        case SEA :
            Fire(workFrequence, 10.0, 50.0, 0.0, 35.0, 1, 150, 0xff);
            break;
    } 
}

void WriteToDataSocket(char* pData, uint16_t length)
{
  if (dataBuffer.m_nToSend == 0)
  {
    dataBuffer.m_nToSend = length;
    dataBuffer.m_pToSend = (char*) realloc (dataBuffer.m_pToSend, length); /* 内存已经清理 */
    memcpy(dataBuffer.m_pToSend, pData, length);
  }
}

void Fire(int mode, double range, double gain, double speedOfSound, double salinity, bool gainAssist, uint8_t gamma, uint8_t netSpeedLimit)
{

    struct _CaiTeamSimpleFireMessage sfm;
    memset(&sfm, 0, sizeof(struct _CaiTeamSimpleFireMessage));

    sfm.head.msgId       = messageSimpleFire;
    sfm.head.srcDeviceId = 0;
    sfm.head.dstDeviceId = 0;
    sfm.head.CaiTeamId    = 0x4f53;

    // Always allow the range to be set as metres
    uint8_t flags = 0x01; 

    if (gainAssist)
        flags |= 0x10; //flagsGainAssist;

    flags |= 0x08; /* 发送简单的返回消息 */

    // ##### Enable 512 beams #####
    flags |= 0x40; 
    // ############################

    sfm.flags = flags;				// Turn on the gain assistance 
    sfm.gammaCorrection = gamma;
    sfm.pingRate      = pingRateNormal;
    sfm.networkSpeed  = netSpeedLimit;
    sfm.masterMode    = mode;
    sfm.range         = range;
    sfm.gainPercent   = gain;

    sfm.speedOfSound = speedOfSound;
    sfm.salinity     = salinity;

    WriteToDataSocket((char*)&sfm, sizeof(struct _CaiTeamSimpleFireMessage));
}

void ProcessRxBuffer(void)
{
    /* 数据包的大小 */
    int64_t pktSize = (int64_t)sizeof(struct _CaiTeamMessageHeader); /* 先获取消息头的大小 */
    if(dataBuffer.m_nRxIn >= pktSize){
        struct _CaiTeamMessageHeader* pOmh = (struct _CaiTeamMessageHeader*) dataBuffer.m_pRxBuffer;
        if(pOmh -> CaiTeamId != 0x4f53){
            dataBuffer.m_nFlushes++;
            printf(" unknow data , throw it \n ");
            dataBuffer.m_nRxIn = 0; /* 这一帧数据作废 */
            return;
        }
        pktSize += (pOmh -> payloadSize); /* earn the total of the package */
    
        if(dataBuffer.m_nRxIn >= pktSize){
            ProcessPayload(dataBuffer.m_pRxBuffer,pktSize); /* 处理有效数据 */
            /* 将处理完剩下的数据往前推 */
            memmove(dataBuffer.m_pRxBuffer,dataBuffer.m_pRxBuffer[dataBuffer.m_nRxIn],dataBuffer.m_nRxIn - pktSize);
            /* 记录接收的数据总共的大小*/
            dataBuffer.m_nRxIn -= pktSize;
        }
    }
}

/* 加载有效数据 */
void ProcessPayload(char* pData, uint64_t nData)
{
  struct _CaiTeamMessageHeader* pOmh = (struct _CaiTeamMessageHeader*) pData;
  /* 我们只看 ping 的结果 */
  if (pOmh->msgId == messageSimplePingResult){
    // Get the next available protected buffer
    struct _OsBufferEntry* pBuffer = &dataBuffer.m_osBuffer[dataBuffer.m_osInject];
    dataBuffer.m_osInject = (dataBuffer.m_osInject + 1) % OS_BUFFER_SIZE;
    /* 将原始数据存为图像数据 */
    ProcessRaw(pData);
    /* 处理完数据则数据更新 */
    NewReturnFire(pBuffer); 
  }
  else if (pOmh->msgId == messageUserConfig) {
	printf("Got a USER CONFIG message");
  }
  // ignore dummy messages
  else if (pOmh->msgId != messageDummy )
        printf("Unrecognised message ID: %d \n " ,pOmh->msgId);
}
/* 处理有效数据 */
void ProcessRaw(char* pData)
{
    if(!pData) return;
    struct _CaiTeamMessageHeader head;
    memcpy(&head, pData, sizeof(struct _CaiTeamMessageHeader));
    switch(head.msgId){
        case messageSimplePingResult:
        {
            OsBufferEntry.m_simple = true; /* 标记采集完成，并且是简单的结果 */
            /* 获取ping的版本 */
            uint16_t ver = head.msgVersion;   /* 版本从何而来？*/
			uint32_t imageSize = 0;
			uint32_t imageOffset = 0;
			uint16_t beams = 0;
			uint32_t size = 0;
            /* 保存版本号 */
            OsBufferEntry.m_version = ver;
            /* 检查是 V1 还是 V2 ping的采样结果 */
            if(ver == 2){
                memcpy(&OsBufferEntry.m_rfm2,pData,sizeof(struct _CaiTeamSimplePingResult2));
                imageSize = OsBufferEntry.m_rfm2.imageSize;
				imageOffset = OsBufferEntry.m_rfm2.imageOffset;
				beams = OsBufferEntry.m_rfm2.nBeams;

				size = sizeof(struct _CaiTeamSimplePingResult2);
            }else{
                memcpy(&OsBufferEntry.m_rfm, pData, sizeof(struct _CaiTeamSimplePingResult));

				imageSize = OsBufferEntry.m_rfm.imageSize;
				imageOffset = OsBufferEntry.m_rfm.imageOffset;
				beams = OsBufferEntry.m_rfm.nBeams;

				size = sizeof(struct _CaiTeamSimplePingResult);
            }
            /* 如果有效数据的总长度等于图片的大小与图片偏移量的和 */
            if((head.payloadSize + sizeof(struct _CaiTeamMessageHeader)) == (imageSize + imageOffset)){

                OsBufferEntry.m_pImage = (u_char *) realloc(OsBufferEntry.m_pImage,imageSize); 
                if( OsBufferEntry.m_pImage )
                    memcpy(OsBufferEntry.m_pImage, pData + imageOffset, imageSize);
                
                // 复制姿态表 
				OsBufferEntry.m_pBrgs = (short*) realloc(OsBufferEntry.m_pBrgs, beams * sizeof(short)); 

				if (OsBufferEntry.m_pBrgs)
				  memcpy(OsBufferEntry.m_pBrgs, pData + size, beams * sizeof(short));
            }else{
                printf("Error in Simple Return Fire Message\n");
            }
            break;
        }
        case messagePingResult:
        {
            printf("got full ping result\n");
            OsBufferEntry.m_simple = false; /* 标记采集完成，并且是全部的结果 */
            memcpy(&OsBufferEntry.m_rfm, pData, sizeof(struct _CaiTeamReturnFireMessage));
            if (OsBufferEntry.m_rff.head.payloadSize + sizeof(struct _CaiTeamMessageHeader) == OsBufferEntry.m_rff.ping_params.imageOffset + OsBufferEntry.m_rff.ping_params.imageSize){
				  // Should be safe to copy the image
				  OsBufferEntry.m_pImage = (u_char*) realloc(OsBufferEntry.m_pImage,  OsBufferEntry.m_rff.ping_params.imageSize);

				  if (OsBufferEntry.m_pImage)
					memcpy(OsBufferEntry.m_pImage, pData + OsBufferEntry.m_rff.ping_params.imageOffset, OsBufferEntry.m_rff.ping_params.imageSize);

				  // Copy the bearing table
				  OsBufferEntry.m_pBrgs = (short*) realloc(OsBufferEntry.m_pBrgs, OsBufferEntry.m_rff.ping.nBeams * sizeof(short));

				  if (OsBufferEntry.m_pBrgs)
					memcpy(OsBufferEntry.m_pBrgs, pData + sizeof(struct _CaiTeamReturnFireMessage), OsBufferEntry.m_rff.ping.nBeams * sizeof(short));
			}else
				printf("Error in Simple Return Fire Message. Byte Match: %d != %d", \
                        (OsBufferEntry.m_rfm.fireMessage.head.payloadSize + sizeof(struct _CaiTeamMessageHeader)) ,(OsBufferEntry.m_rfm.imageOffset + OsBufferEntry.m_rfm.imageSize));

            break;
        }
    }
}

void NewReturnFire(struct _OsBufferEntry* pEntry)
{
    uint16_t dst = 0;
    int width = 0;
    int height = 0;
    double range = 0;
    uint16_t ver = 0;

    if(pEntry->m_simple){
        if(pEntry->m_version == 2){
            width = pEntry->m_rfm2.nBeams;
            height = pEntry->m_rfm2.nRanges;
            range = height * pEntry->m_rfm2.rangeResolution;
            dst = pEntry->m_rfm2.fireMessage.head.srcDeviceId;
        } else {
            width = pEntry->m_rfm.nBeams;
            height = pEntry->m_rfm.nRanges;
            range = height * pEntry->m_rfm.rangeResolution;
            dst = pEntry->m_rfm.fireMessage.head.srcDeviceId;
        }
    } else {
          width  = pEntry->m_rff.ping.nBeams;
          height = pEntry->m_rff.ping_params.nRangeLinesBfm;
          range = pEntry->m_rff.ping.range;
          dst = pEntry->m_rff.head.srcDeviceId;
          ver = pEntry->m_rff.head.msgVersion;
    }

    /* 得到图像的新数据 */
    printf("\n image data update  \n");
    printf(  "dst = %d , width = %d ,height = %d , range = %lf, ver = %d \n" , dst , width , height , range , ver);

    /* 对应QT源代码 */
    // m_pSonarSurface->UpdateFan(range, width, pEntry->m_pBrgs, true); /* 更新扇形图数据 */
    // m_pSonarSurface->UpdateImg(height, width, pEntry->m_pImage); /* 更新图片数据 */
    // m_display_widget->m_image_thread->get_data(pEntry->m_pImage,height,width,dst,range);
        /* 释放内存 */
    free(OsBufferEntry.m_pBrgs);
    free(OsBufferEntry.m_pImage);
    OsBufferEntry.m_pBrgs = NULL;
    OsBufferEntry.m_pImage = NULL;
    /* 继续发送数据给声纳 */
    /* code */
    FireSonar(lowFreMode); /* 低频率 淡水 */
}
/*
 * @Author: oyw 1622945822@qq.com
 * @Date: 2023-11-24 14:32:21
 * @LastEditors: oyw 1622945822@qq.com
 * @LastEditTime: 2023-11-30 19:44:54
 * @FilePath: \sonarCode\process.c
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "process.h"

/* mode Ϊ����Ƶ�ʵĿ��� */
void FireSonar(int workFrequence)
{
    /* д������Ĭ�����ò��� */
    switch(WATERENVI){ /* Ĭ���ڵ�ˮִ������ */
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
    dataBuffer.m_pToSend = (char*) realloc (dataBuffer.m_pToSend, length); /* �ڴ��Ѿ����� */
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

    flags |= 0x08; /* ���ͼ򵥵ķ�����Ϣ */

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
    /* ���ݰ��Ĵ�С */
    int64_t pktSize = (int64_t)sizeof(struct _CaiTeamMessageHeader); /* �Ȼ�ȡ��Ϣͷ�Ĵ�С */
    if(dataBuffer.m_nRxIn >= pktSize){
        struct _CaiTeamMessageHeader* pOmh = (struct _CaiTeamMessageHeader*) dataBuffer.m_pRxBuffer;
        if(pOmh -> CaiTeamId != 0x4f53){
            dataBuffer.m_nFlushes++;
            printf(" unknow data , throw it \n ");
            dataBuffer.m_nRxIn = 0; /* ��һ֡�������� */
            return;
        }
        pktSize += (pOmh -> payloadSize); /* earn the total of the package */
    
        if(dataBuffer.m_nRxIn >= pktSize){
            ProcessPayload(dataBuffer.m_pRxBuffer,pktSize); /* ������Ч���� */
            /* ��������ʣ�µ�������ǰ�� */
            memmove(dataBuffer.m_pRxBuffer,dataBuffer.m_pRxBuffer[dataBuffer.m_nRxIn],dataBuffer.m_nRxIn - pktSize);
            /* ��¼���յ������ܹ��Ĵ�С*/
            dataBuffer.m_nRxIn -= pktSize;
        }
    }
}

/* ������Ч���� */
void ProcessPayload(char* pData, uint64_t nData)
{
  struct _CaiTeamMessageHeader* pOmh = (struct _CaiTeamMessageHeader*) pData;
  /* ����ֻ�� ping �Ľ�� */
  if (pOmh->msgId == messageSimplePingResult){
    // Get the next available protected buffer
    struct _OsBufferEntry* pBuffer = &dataBuffer.m_osBuffer[dataBuffer.m_osInject];
    dataBuffer.m_osInject = (dataBuffer.m_osInject + 1) % OS_BUFFER_SIZE;
    /* ��ԭʼ���ݴ�Ϊͼ������ */
    ProcessRaw(pData);
    /* ���������������ݸ��� */
    NewReturnFire(pBuffer); 
  }
  else if (pOmh->msgId == messageUserConfig) {
	printf("Got a USER CONFIG message");
  }
  // ignore dummy messages
  else if (pOmh->msgId != messageDummy )
        printf("Unrecognised message ID: %d \n " ,pOmh->msgId);
}
/* ������Ч���� */
void ProcessRaw(char* pData)
{
    if(!pData) return;
    struct _CaiTeamMessageHeader head;
    memcpy(&head, pData, sizeof(struct _CaiTeamMessageHeader));
    switch(head.msgId){
        case messageSimplePingResult:
        {
            OsBufferEntry.m_simple = true; /* ��ǲɼ���ɣ������Ǽ򵥵Ľ�� */
            /* ��ȡping�İ汾 */
            uint16_t ver = head.msgVersion;   /* �汾�Ӻζ�����*/
			uint32_t imageSize = 0;
			uint32_t imageOffset = 0;
			uint16_t beams = 0;
			uint32_t size = 0;
            /* ����汾�� */
            OsBufferEntry.m_version = ver;
            /* ����� V1 ���� V2 ping�Ĳ������ */
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
            /* �����Ч���ݵ��ܳ��ȵ���ͼƬ�Ĵ�С��ͼƬƫ�����ĺ� */
            if((head.payloadSize + sizeof(struct _CaiTeamMessageHeader)) == (imageSize + imageOffset)){

                OsBufferEntry.m_pImage = (u_char *) realloc(OsBufferEntry.m_pImage,imageSize); 
                if( OsBufferEntry.m_pImage )
                    memcpy(OsBufferEntry.m_pImage, pData + imageOffset, imageSize);
                
                // ������̬�� 
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
            OsBufferEntry.m_simple = false; /* ��ǲɼ���ɣ�������ȫ���Ľ�� */
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

    /* �õ�ͼ��������� */
    printf("\n image data update  \n");
    printf(  "dst = %d , width = %d ,height = %d , range = %lf, ver = %d \n" , dst , width , height , range , ver);

    /* ��ӦQTԴ���� */
    // m_pSonarSurface->UpdateFan(range, width, pEntry->m_pBrgs, true); /* ��������ͼ���� */
    // m_pSonarSurface->UpdateImg(height, width, pEntry->m_pImage); /* ����ͼƬ���� */
    // m_display_widget->m_image_thread->get_data(pEntry->m_pImage,height,width,dst,range);
        /* �ͷ��ڴ� */
    free(OsBufferEntry.m_pBrgs);
    free(OsBufferEntry.m_pImage);
    OsBufferEntry.m_pBrgs = NULL;
    OsBufferEntry.m_pImage = NULL;
    /* �����������ݸ����� */
    /* code */
    FireSonar(lowFreMode); /* ��Ƶ�� ��ˮ */
}
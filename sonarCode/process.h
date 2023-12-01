#ifndef _PROCESS_H
#define _PROCESS_H

#include "table.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

void FireSonar(int workFrequence);
void WriteToDataSocket(char* pData, uint16_t length);
void Fire(int mode, double range, double gain, double speedOfSound, double salinity, bool gainAssist, uint8_t gamma, uint8_t netSpeedLimit);
void ProcessRxBuffer(void);
void ProcessPayload(char* pData, uint64_t nData);
void ProcessRaw(char* pData);

#endif
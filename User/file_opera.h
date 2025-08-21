#ifndef __FILE_OPERA_H
#define __FILE_OPERA_H

#include "fatfs.h"
#include "stdint.h"

void fatTest_GetDiskInfo(void);
void fatTest_ScanDir(const TCHAR* PathName);
void fatTest_WriteTXTFile(TCHAR* filename, uint16_t year, uint8_t month, uint8_t day);
void fatTest_WriteBinFile(TCHAR* filename, uint32_t pointCount, uint32_t sampFreq);
void fatTest_ReadTXTFile(TCHAR* filename);
void fatTest_ReadBinFile(TCHAR* filename);
void fatTest_GetFileInfo(TCHAR* filename);
DWORD fat_GetFatTimeFromRTC(void);
void fatTest_RemoveAll(void);

#endif /* __FILE_OPERA_H */

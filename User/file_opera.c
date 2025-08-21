#include "file_opera.h"
#include "fatfs.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 声明外部变量
extern RTC_HandleTypeDef hrtc;

void fatTest_GetDiskInfo(void)
{
    FATFS *fs;
    DWORD free_clust;
    FRESULT res = f_getfree("0:", &free_clust, &fs);
    if (res != FR_OK)
    {
        printf("Error: f_getfree() failed with code %d\r\n", res);
        return;
    }

    printf("\r\n=== FAT File System Information ===\r\n");
    DWORD total_sector = (fs->n_fatent - 2) * fs->csize;
    DWORD free_sector = free_clust * fs->csize;

#if _MAX_SS == _MIN_SS
    DWORD free_space = free_sector >> 11;
    DWORD total_space = total_sector >> 11;
#else
    DWORD free_space = (free_sector * FF_MIN_SS) >> 10;
    DWORD total_space = (total_sector * FF_MIN_SS) >> 10;
#endif

    const char *fs_type_str[] = {"Unknown", "FAT12", "FAT16", "FAT32", "EXFAT"};
    int fs_type_index = (fs->fs_type < 1 || fs->fs_type > 4) ? 0 : fs->fs_type;

    printf("File System Type     : %s (Type %d)\r\n", fs_type_str[fs_type_index], fs->fs_type);
    printf("Sector Size          : %d bytes\r\n", _MIN_SS);
    printf("Cluster Size         : %d sectors (%d bytes)\r\n", fs->csize, fs->csize * _MIN_SS);
    printf("Total Clusters       : %d\r\n", fs->n_fatent - 2);
    printf("Free Clusters        : %d\r\n", free_clust);
    printf("Total Sectors        : %d\r\n", total_sector);
    printf("Free Sectors         : %d\r\n", free_sector);
    printf("Total Space          : %d MB\r\n", total_space);
    printf("Free Space           : %d MB\r\n", free_space);
    printf("Used Space           : %d MB\r\n", total_space - free_space);
    printf("==================================\r\n\r\n");
}

void fatTest_ScanDir(const TCHAR *PathName)
{
    // 使用动态内存分配
    DIR *dir = (DIR *)malloc(sizeof(DIR));
    FILINFO *dir_info = (FILINFO *)malloc(sizeof(FILINFO));

    if (dir == NULL || dir_info == NULL)
    {
        printf("Error: Not enough memory for directory scan\r\n");
        if (dir)
            free(dir);
        if (dir_info)
            free(dir_info);
        return;
    }

    FRESULT res = f_opendir(dir, PathName);
    if (res != FR_OK)
    {
        printf("Error: Failed to open directory '%s' (Error code: %d)\r\n", PathName, res);
        free(dir);
        free(dir_info);
        return;
    }

    printf("\r\n=== Directory Contents: %s ===\r\n", PathName);
    printf("%-8s %-12s %-20s\r\n", "Type", "Size", "Name");
    printf("----------------------------------------\r\n");

    uint32_t file_count = 0;
    uint32_t dir_count = 0;
    uint64_t total_size = 0;

    while (1)
    {
        res = f_readdir(dir, dir_info);
        if (res != FR_OK || dir_info->fname[0] == 0)
        {
            break;
        }

        if (dir_info->fname[0] == '.' && (dir_info->fname[1] == '\0' ||
                                          (dir_info->fname[1] == '.' && dir_info->fname[2] == '\0')))
        {
            // Skip "." and ".." entries
            continue;
        }

        if (dir_info->fattrib & AM_DIR)
        {
            printf("%-8s %-12s %-20s\r\n", "<DIR>", "", dir_info->fname);
            dir_count++;
        }
        else
        {
            printf("%-8s %-12u %-20s\r\n", "<FILE>", (unsigned int)dir_info->fsize, dir_info->fname);
            file_count++;
            total_size += dir_info->fsize;
        }
    }

    printf("----------------------------------------\r\n");
    printf("Files: %d, Directories: %d, Total Size: %llu bytes\r\n",
           file_count, dir_count, total_size);
    printf("Directory scan completed successfully\r\n");
    printf("========================================\r\n\r\n");

    f_closedir(dir);
    free(dir);
    free(dir_info);
}

void fatTest_WriteTXTFile(TCHAR *filename, uint16_t year, uint8_t month, uint8_t day)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK)
    {
        f_puts("Line1: Hello FatFS\r\n", &file);
        f_puts("Line2: UPC, Qingdao\r\n", &file);
        f_printf(&file, "Line3: Date=%04d-%02d-%02d\r\n", year, month, day);
        f_close(&file);
        printf("Success: Text file '%s' created successfully\r\n", filename);
    }
    else
    {
        printf("Error: Failed to create text file '%s' (Error code: %d)\r\n", filename, res);
    }
}

void fatTest_WriteBinFile(TCHAR *filename, uint32_t pointCount, uint32_t sampFreq)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK)
    {
        f_puts("ADC1-IN5\r\n", &file);

        UINT bw = 0;
        f_write(&file, &pointCount, sizeof(uint32_t), &bw);
        f_write(&file, &sampFreq, sizeof(uint32_t), &bw);

        uint32_t value = 1000;
        for (uint16_t i = 0; i < pointCount; i++, value++)
        {
            f_write(&file, &value, sizeof(uint32_t), &bw);
        }
        f_close(&file);
        printf("Success: Binary file '%s' created successfully\r\n", filename);
        printf("         Points: %d, Sampling Frequency: %d Hz\r\n", (int)pointCount, (int)sampFreq);
    }
    else
    {
        printf("Error: Failed to create binary file '%s' (Error code: %d)\r\n", filename, res);
    }
}

void fatTest_ReadTXTFile(TCHAR *filename)
{
    printf("\r\n=== Reading Text File: %s ===\r\n", filename);

    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res == FR_OK)
    {
        TCHAR show_str[100];
        uint32_t line_num = 1;
        printf("Content:\r\n");
        while (!f_eof(&file))
        {
            if (f_gets(show_str, sizeof(show_str), &file) != NULL)
            {
                printf("%3d: %s", line_num++, show_str);
            }
        }
        printf("End of file\r\n");
        printf("File read completed successfully\r\n");
    }
    else if (res == FR_NO_FILE)
    {
        printf("Error: File '%s' does not exist\r\n", filename);
    }
    else
    {
        printf("Error: Failed to open file '%s' (Error code: %d)\r\n", filename, res);
    }
    f_close(&file);
    printf("===========================================\r\n\r\n");
}

void fatTest_ReadBinFile(TCHAR *filename)
{
    printf("\r\n=== Reading Binary File: %s ===\r\n", filename);
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res == FR_OK)
    {
        TCHAR show_str[40];
        if (f_gets(show_str, sizeof(show_str), &file) != NULL)
        {
            printf("Header: %s", show_str);
        }

        UINT bw = 0;
        uint32_t pointCount;
        uint32_t sampFreq;
        f_read(&file, &pointCount, sizeof(uint32_t), &bw);
        f_read(&file, &sampFreq, sizeof(uint32_t), &bw);

        printf("Point Count: %d\r\n", (int)pointCount);
        printf("Sampling Frequency: %d Hz\r\n", (int)sampFreq);

        if (pointCount > 1000)
        {
            printf("Warning: Point count is too large, limiting to 1000\r\n");
            pointCount = 1000;
        }

        uint32_t *value = (uint32_t *)malloc(pointCount * sizeof(uint32_t));
        if (value != NULL)
        {
            uint32_t read_count = 0;
            for (uint16_t i = 0; i < pointCount; ++i)
            {
                if (f_read(&file, &value[i], sizeof(uint32_t), &bw) == FR_OK && bw == sizeof(uint32_t))
                {
                    read_count++;
                }
                else
                {
                    break;
                }
            }

            printf("Successfully read %d data points\r\n", read_count);

            // Show some sample data
            printf("Sample data points:\r\n");
            for (int i = 0; i < 10 && i < read_count; i++)
            {
                printf("  [%d] = %d\r\n", i, (int)value[i]);
            }
            if (read_count > 10)
            {
                printf("  ...\r\n");
                printf("  [%d] = %d\r\n", read_count - 1, (int)value[read_count - 1]);
            }

            free(value);
            printf("File read completed successfully\r\n");
        }
        else
        {
            printf("Error: Failed to allocate memory for %d points\r\n", (int)pointCount);
        }
    }
    else if (res == FR_NO_FILE)
    {
        printf("Error: File '%s' does not exist\r\n", filename);
    }
    else
    {
        printf("Error: Failed to open file '%s' (Error code: %d)\r\n", filename, res);
    }
    f_close(&file);
    printf("====================================\r\n\r\n");
}

void fatTest_GetFileInfo(TCHAR *filename)
{
    printf("\r\n=== File Information: %s ===\r\n", filename);

    FILINFO file_info;
    FRESULT res = f_stat(filename, &file_info);
    if (res == FR_OK)
    {
        printf("File Name            : %s\r\n", file_info.fname);
        printf("File Size            : %u bytes\r\n", (unsigned int)file_info.fsize);
        printf("File Attributes      : 0x%02X", file_info.fattrib);

        // Parse attributes
        printf(" (");
        if (file_info.fattrib & AM_RDO)
            printf("R");
        else
            printf("r");

        if (file_info.fattrib & AM_HID)
            printf("H");
        else
            printf("h");

        if (file_info.fattrib & AM_SYS)
            printf("S");
        else
            printf("s");

        if (file_info.fattrib & AM_ARC)
            printf("A");
        else
            printf("a");

        if (file_info.fattrib & AM_DIR)
            printf(",DIR");
        printf(")\r\n");

        // Parse date and time
        uint16_t year = 1980 + ((file_info.fdate & 0xFE00) >> 9);
        uint8_t month = (file_info.fdate & 0x01E0) >> 5;
        uint8_t day = file_info.fdate & 0x001F;

        uint8_t hour = (file_info.ftime & 0xF800) >> 11;
        uint8_t minute = (file_info.ftime & 0x07E0) >> 5;
        uint8_t second = (file_info.ftime & 0x001F) << 1;

        printf("Creation Date        : %04d-%02d-%02d\r\n", year, month, day);
        printf("Creation Time        : %02d:%02d:%02d\r\n", hour, minute, second);
        printf("File information retrieved successfully\r\n");
    }
    else if (res == FR_NO_FILE)
    {
        printf("Error: File '%s' does not exist\r\n", filename);
    }
    else
    {
        printf("Error: Failed to get file info for '%s' (Error code: %d)\r\n", filename, res);
    }
    printf("=====================================\r\n\r\n");
}

DWORD fat_GetFatTimeFromRTC(void)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
    {
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        WORD date = ((2000 + sDate.Year - 1980) << 9) | (sDate.Month << 5) | sDate.Date;
        WORD time = (sTime.Hours << 11) | (sTime.Minutes << 5) | (sTime.Seconds >> 1);

        DWORD fatTime = ((DWORD)date << 16) | time;
        printf("Current RTC Time: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
               sDate.Year, sDate.Month, sDate.Date,
               sTime.Hours, sTime.Minutes, sTime.Seconds);
        return fatTime;
    }
    else
    {
        printf("Error: Failed to get time from RTC\r\n");
        return 0;
    }
}

void fatTest_RemoveAll(void)
{
    DIR dir;
    FRESULT res = f_opendir(&dir, "0:/");
    if (res != FR_OK)
    {
        printf("Error: Failed to open root directory (Error code: %d)\r\n", res);
        f_closedir(&dir);
        return;
    }

    printf("\r\n=== Removing All Files ===\r\n");
    FILINFO fno;
    uint32_t file_count = 0;
    uint32_t dir_count = 0;

    while (1)
    {
        res = f_readdir(&dir, &fno);
        if ((res != FR_OK) || (fno.fname[0] == '\0'))
        {
            break;
        }

        // Skip "." and ".."
        if (fno.fname[0] == '.' && (fno.fname[1] == '\0' ||
                                    (fno.fname[1] == '.' && fno.fname[2] == '\0')))
        {
            continue;
        }

        // Skip directories for now (deleting files only)
        if (fno.fattrib & AM_DIR)
        {
            printf("Skipped directory: %s\r\n", fno.fname);
            dir_count++;
            continue;
        }

        FRESULT del_res = f_unlink(fno.fname);
        if (del_res == FR_OK)
        {
            printf("Deleted file: %s\r\n", fno.fname);
            file_count++;
        }
        else
        {
            printf("Failed to delete file: %s (Error code: %d)\r\n", fno.fname, del_res);
        }
    }

    f_closedir(&dir);
    printf("Operation completed: %d files deleted, %d directories skipped\r\n", file_count, dir_count);
    printf("==========================\r\n\r\n");
}

#include "lfs_spi_flash_adapter.h"
#include <stdio.h>
#include <string.h>
#include "w25q128.h" // 包含W25Q128头文件用于直接访问

// 获取错误信息描述
const char *lfs_get_error_string(int err)
{
    switch (err)
    {
    case LFS_ERR_OK:
        return "No error\r\n";
    case LFS_ERR_IO:
        return "Error during device operation\r\n";
    case LFS_ERR_CORRUPT:
        return "Corrupted filesystem\r\n";
    case LFS_ERR_NOENT:
        return "No directory entry (file not found)\r\n";
    case LFS_ERR_EXIST:
        return "Entry already exists\r\n";
    case LFS_ERR_NOTDIR:
        return "Entry is not a directory\r\n";
    case LFS_ERR_ISDIR:
        return "Entry is a directory\r\n";
    case LFS_ERR_NOTEMPTY:
        return "Directory is not empty\r\n";
    case LFS_ERR_BADF:
        return "Bad file number\r\n";
    case LFS_ERR_FBIG:
        return "File too large\r\n";
    case LFS_ERR_INVAL:
        return "Invalid parameter\r\n";
    case LFS_ERR_NOSPC:
        return "No space left on device\r\n";
    case LFS_ERR_NOMEM:
        return "No more memory available\r\n";
    case LFS_ERR_NOATTR:
        return "No data/attribute available\r\n";
    case LFS_ERR_NAMETOOLONG:
        return "File name too long\r\n";
    default:
        return "Unknown error\r\n";
    }
}

// 测试函数
void lfs_test(void)
{
    int err;

    // 初始化SPI Flash
    printf("Initializing SPI Flash...\r\n");
    err = lfs_spi_flash_init();
    if (err != 0)
    {
        printf("SPI Flash initialization failed! Error: %d\r\n", err);
        return;
    }
    printf("SPI Flash initialized successfully.\r\n");

    // 读取并显示SPI Flash ID作为附加验证
    uint16_t flash_id = W25Q128_readID();
    printf("SPI Flash ID: 0x%04X\r\n", flash_id);

    // 显示LittleFS配置信息
    printf("LittleFS Configuration:\r\n");
    printf("  Read size: %d bytes\r\n", lfs_spi_flash_cfg.read_size);
    printf("  Program size: %d bytes\r\n", lfs_spi_flash_cfg.prog_size);
    printf("  Block size: %d bytes\r\n", lfs_spi_flash_cfg.block_size);
    printf("  Block count: %d\r\n", lfs_spi_flash_cfg.block_count);
    printf("  Total size: %d KB\r\n", (lfs_spi_flash_cfg.block_size * lfs_spi_flash_cfg.block_count) / 1024);

    // 挂载文件系统
    printf("Mounting littlefs...\r\n");
    err = lfs_spi_flash_mount(NULL); // 使用NULL让函数使用内部的全局实例
    if (err != LFS_ERR_OK)
    {
        printf("Failed to mount littlefs: %d - %s\r\n", err, lfs_get_error_string(err));
        return;
    }
    printf("littlefs mounted successfully.\r\n");

    // 挂载后列出目录内容
    printf("Listing initial directory contents...\r\n");
    struct lfs_dir dir;
    err = lfs_dir_open(&lfs_instance, &dir, "/");
    if (err == LFS_ERR_OK)
    {
        struct lfs_info info;
        int found_files = 0;
        while (lfs_dir_read(&lfs_instance, &dir, &info) > 0)
        {
            printf("  Found: %s (type: %d)\r\n", info.name, info.type);
            found_files++;
        }
        if (found_files == 0)
        {
            printf("  Directory is empty\r\n");
        }
        lfs_dir_close(&lfs_instance, &dir);
    }
    else
    {
        printf("  Failed to open directory: %d - %s\r\n", err, lfs_get_error_string(err));
    }

    // 测试文件写入
    printf("Testing file write...\r\n");
    struct lfs_file file;
    const char *filename = "test.txt";

    // 确保使用完整路径
    err = lfs_file_open(&lfs_instance, &file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != LFS_ERR_OK)
    {
        printf("Failed to open file: %d - %s\r\n", err, lfs_get_error_string(err));
        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }
    printf("Successfully opened file for writing\r\n");

    const char *test_data = "Hello, littlefs on SPI Flash!";
    int expected_size = strlen(test_data);
    err = lfs_file_write(&lfs_instance, &file, test_data, expected_size);
    if (err < 0)
    {
        printf("Failed to write file: %d - %s\r\n", err, lfs_get_error_string(err));
        lfs_file_close(&lfs_instance, &file);
        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }
    printf("Written %d of %d expected bytes to file\r\n", err, expected_size);

    // 检查写入大小是否符合预期
    if (err != expected_size)
    {
        printf("Warning: Wrote %d bytes but expected %d bytes\r\n", err, expected_size);
    }

    // 确保数据被刷新到存储
    err = lfs_file_sync(&lfs_instance, &file);
    if (err != LFS_ERR_OK)
    {
        printf("Failed to sync file: %d - %s\r\n", err, lfs_get_error_string(err));
        lfs_file_close(&lfs_instance, &file);
        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }
    printf("File sync completed successfully\r\n");

    err = lfs_file_close(&lfs_instance, &file);
    if (err != LFS_ERR_OK)
    {
        printf("Failed to close file: %d - %s\r\n", err, lfs_get_error_string(err));
        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }
    printf("File closed successfully\r\n");

    // 注意：LittleFS会在文件关闭和文件系统卸载时自动同步数据
    // 当前版本的LittleFS不提供lfs_sync函数
    printf("Data will be automatically synced during file close and filesystem unmount\r\n");

    // 列出目录内容以确认文件是否存在
    printf("Listing directory contents after write...\r\n");
    err = lfs_dir_open(&lfs_instance, &dir, "/");
    if (err == LFS_ERR_OK)
    {
        struct lfs_info info;
        int found_file = 0;
        while (lfs_dir_read(&lfs_instance, &dir, &info) > 0)
        {
            printf("  Found: %s (type: %d, size: %d)\r\n", info.name, info.type, info.size);
            if (strcmp(info.name, filename) == 0)
            {
                found_file = 1;
                printf("Found target file: %s (size: %d bytes)\r\n", filename, info.size);
            }
        }
        if (!found_file)
        {
            printf("Could not find target file: %s\r\n", filename);
        }
        lfs_dir_close(&lfs_instance, &dir);
    }
    else
    {
        printf("  Failed to open directory: %d - %s\r\n", err, lfs_get_error_string(err));
    }

    // 测试文件读取
    printf("Testing file read...\r\n");
    err = lfs_file_open(&lfs_instance, &file, filename, LFS_O_RDONLY);
    if (err != LFS_ERR_OK)
    {
        printf("Failed to open file for reading: %d - %s\r\n", err, lfs_get_error_string(err));

        // 详细调试信息
        printf("Detailed debug information:\r\n");
        printf("  Error code LFS_ERR_NOENT: %d\r\n", LFS_ERR_NOENT);
        printf("  File system pointer: %p\r\n", &lfs_instance);
        printf("  File pointer: %p\r\n", &file);
        printf("  Attempting to open root directory again...\r\n");

        err = lfs_dir_open(&lfs_instance, &dir, "/");
        if (err == LFS_ERR_OK)
        {
            struct lfs_info info;
            printf("  Root directory listing:\r\n");
            while (lfs_dir_read(&lfs_instance, &dir, &info) > 0)
            {
                printf("    %s (type: %d, size: %d)\r\n", info.name, info.type, info.size);
            }
            lfs_dir_close(&lfs_instance, &dir);
        }
        else
        {
            printf("    Failed to open directory: %d - %s\r\n", err, lfs_get_error_string(err));
        }

        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }
    printf("Successfully opened file for reading\r\n");

    char read_buffer[100] = {0};
    err = lfs_file_read(&lfs_instance, &file, read_buffer, sizeof(read_buffer) - 1);
    if (err < 0)
    {
        printf("Failed to read file: %d - %s\r\n", err, lfs_get_error_string(err));
        lfs_file_close(&lfs_instance, &file);
        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }
    printf("Read %d bytes from file: %s\r\n", err, read_buffer);

    err = lfs_file_close(&lfs_instance, &file);
    if (err != LFS_ERR_OK)
    {
        printf("Failed to close file: %d - %s\r\n", err, lfs_get_error_string(err));
        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }

    // 测试删除文件
    printf("Testing file delete...\r\n");
    err = lfs_remove(&lfs_instance, "test.txt");
    if (err != LFS_ERR_OK)
    {
        printf("Failed to delete file: %d - %s\r\n", err, lfs_get_error_string(err));
        lfs_spi_flash_unmount(&lfs_instance);
        return;
    }
    printf("File deleted successfully.\r\n");

    // 卸载文件系统
    printf("Unmounting littlefs...\r\n");
    err = lfs_spi_flash_unmount(&lfs_instance);
    if (err != LFS_ERR_OK)
    {
        printf("Failed to unmount littlefs: %d - %s\r\n", err, lfs_get_error_string(err));
        return;
    }
    printf("littlefs unmounted successfully.\r\n");

    printf("littlefs test completed!\r\n");
}

/*
 * lfs_spi_flash_adapter.h
 *
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-10     AI助手       first version
 */

#ifndef LFS_SPI_FLASH_ADAPTER_H
#define LFS_SPI_FLASH_ADAPTER_H

#include "lfs.h"
#include "w25q128.h"

// 定义SPI Flash的块大小、扇区大小等参数
#define SPI_FLASH_BLOCK_SIZE 4096  // 块大小，W25Q128一个扇区为4KB
#define SPI_FLASH_BLOCK_COUNT 1024 // 块数量，设置为4MB存储空间，4MB / 4KB = 1024块
#define SPI_FLASH_PROG_SIZE 256    // 编程大小，W25Q128一页为256字节
#define SPI_FLASH_READ_SIZE 1      // 读取大小，最小为1字节

// 声明lfs配置结构体和实例
extern const struct lfs_config lfs_spi_flash_cfg;
extern struct lfs lfs_instance;

// 初始化SPI Flash和littlefs
int lfs_spi_flash_init(void);

// 挂载文件系统
int lfs_spi_flash_mount(struct lfs *lfs);

// 卸载文件系统
int lfs_spi_flash_unmount(struct lfs *lfs);

// 格式化文件系统
int lfs_spi_flash_format(struct lfs *lfs);

#endif /* LFS_SPI_FLASH_ADAPTER_H */

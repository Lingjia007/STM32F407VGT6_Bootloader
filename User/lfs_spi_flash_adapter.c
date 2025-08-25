/*
 * lfs_spi_flash_adapter.c
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

#include "lfs_spi_flash_adapter.h"
#include <string.h>

// 定义lfs句柄
struct lfs lfs_instance;

// SPI Flash块设备操作函数

// 读取数据
static int lfs_spi_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    // 计算实际地址
    uint32_t addr = block * SPI_FLASH_BLOCK_SIZE + off;
    
    // 检查地址范围
    if (block >= SPI_FLASH_BLOCK_COUNT || (off + size) > SPI_FLASH_BLOCK_SIZE) {
        return LFS_ERR_INVAL;
    }

    // 调用W25Q128的读取函数
    W25Q128_read((uint8_t *)buffer, addr, size);

    return LFS_ERR_OK;
}

// 写入数据
static int lfs_spi_flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    // 计算实际地址
    uint32_t addr = block * SPI_FLASH_BLOCK_SIZE + off;
    
    // 检查地址范围
    if (block >= SPI_FLASH_BLOCK_COUNT || (off + size) > SPI_FLASH_BLOCK_SIZE) {
        return LFS_ERR_INVAL;
    }

    // 对于LittleFS，擦除操作由lfs_spi_flash_erase单独处理
    // 所以这里我们只进行写入操作，不执行擦除
    // 先发送写使能
    W25Q128_write_enable();
    W25Q128_wait_busy();
    
    // 拉低CS端为低电平
    W25QXX_CS_ON(1);
    // 发送指令02h
    spi_read_write_byte(W25X_PageProgram);
    // 发送写入的24位地址中的高8位
    spi_read_write_byte((uint8_t)((addr) >> 16));
    // 发送写入的24位地址中的中8位
    spi_read_write_byte((uint8_t)((addr) >> 8));
    // 发送写入的24位地址中的低8位
    spi_read_write_byte((uint8_t)addr);
    // 根据写入的字节长度连续写入数据buffer
    for (uint32_t i = 0; i < size; i++)
    {
        spi_read_write_byte(((uint8_t *)buffer)[i]);
    }
    // 恢复CS端为高电平
    W25QXX_CS_ON(0);
    // 忙检测
    W25Q128_wait_busy();

    return LFS_ERR_OK;
}

// 擦除块
static int lfs_spi_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    // 检查块号范围
    if (block >= SPI_FLASH_BLOCK_COUNT) {
        return LFS_ERR_INVAL;
    }
    
    // 注意：W25Q128_erase_sector函数内部会将输入值乘以4096
    // 我们需要直接传递块号，而不是计算后的地址
    W25Q128_erase_sector(block);

    return LFS_ERR_OK;
}

// 同步操作
static int lfs_spi_flash_sync(const struct lfs_config *c)
{
    // 对于SPI Flash，确保所有操作都已完成
    W25Q128_wait_busy();
    
    // 可选：执行额外的同步操作以确保元数据被正确提交
    return LFS_ERR_OK;
}

// lfs配置结构体
const struct lfs_config lfs_spi_flash_cfg = {
    .context = NULL,
    .read = lfs_spi_flash_read,
    .prog = lfs_spi_flash_prog,
    .erase = lfs_spi_flash_erase,
    .sync = lfs_spi_flash_sync,
    .read_size = SPI_FLASH_READ_SIZE,
    .prog_size = SPI_FLASH_PROG_SIZE,
    .block_size = SPI_FLASH_BLOCK_SIZE,
    .block_count = SPI_FLASH_BLOCK_COUNT,
    .cache_size = SPI_FLASH_PROG_SIZE,
    .lookahead_size = 16,
    .block_cycles = 500,
};

// 初始化SPI Flash和littlefs
int lfs_spi_flash_init(void)
{
    // 初始化W25Q128
    w25q128_init();

    // 检查SPI Flash是否正常工作
    uint16_t flash_id = W25Q128_readID();
    if (flash_id == 0xFFFF || flash_id == 0x0000)
    {
        return -1; // SPI Flash初始化失败
    }

    return 0; // 初始化成功
}

// 挂载文件系统
int lfs_spi_flash_mount(struct lfs *lfs)
{
    // 如果提供了lfs指针就使用它，否则使用全局实例
    struct lfs *target_lfs = (lfs != NULL) ? lfs : &lfs_instance;

    // 尝试挂载文件系统
    int err = lfs_mount(target_lfs, &lfs_spi_flash_cfg);

    // 如果挂载失败，尝试格式化后再挂载
    if (err != LFS_ERR_OK)
    {
        err = lfs_format(target_lfs, &lfs_spi_flash_cfg);
        if (err != LFS_ERR_OK)
        {
            return err;
        }

        err = lfs_mount(target_lfs, &lfs_spi_flash_cfg);
    }

    return err;
}

// 卸载文件系统
int lfs_spi_flash_unmount(struct lfs *lfs)
{
    // 如果提供了lfs指针就使用它，否则使用全局实例
    struct lfs *target_lfs = (lfs != NULL) ? lfs : &lfs_instance;

    return lfs_unmount(target_lfs);
}

// 格式化文件系统
int lfs_spi_flash_format(struct lfs *lfs)
{
    // 如果提供了lfs指针就使用它，否则使用全局实例
    struct lfs *target_lfs = (lfs != NULL) ? lfs : &lfs_instance;

    return lfs_format(target_lfs, &lfs_spi_flash_cfg);
}

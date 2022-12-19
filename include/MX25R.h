/**
 * @file MX25R.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-12-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once 

#ifndef MX25R_H
#define MX25R_H

#include <stdint.h>
#include <stdbool.h>

#define MX25R_PAGE_SIZE         256     ///< 2 ^ 8
#define MX25R_SECTOR_SIZE       2048    ///< 2 ^ 11
#define MX25R_SMALL_BLOCK_SIZE  32768   ///< 2 ^ 15
#define MX25R_BLOCK_SIZE        65536   ///< 2 ^ 16

typedef enum MX25RCOMMAND {

   MX25R_READ               = 0x03,
   MX25R_FAST_READ          = 0x0B,
   MX25R_DOUBLE_READ        = 0xBB,
   MX25R_DREAD              = 0x3B,
   MX25R_QUAD_READ          = 0xEB,
   MX25R_QREAD              = 0x6B,
   MX25R_PAGE_PROG          = 0x02,
   MX25R_QPAGE_PROG         = 0x38,
   MX25R_SECT_ERASE         = 0x20,
   MX25R_BLOCK_ERASE32K     = 0x52,
   MX25R_BLOCK_ERASE        = 0xD8,
   MX25R_CHIP_ERASE         = 0x60,
   MX25R_FLASH_ERASE        = 0xC7,
   MX25R_READ_SFDP          = 0x5A,
   MX25R_WRITE_EN           = 0x06,
   MX25R_WRITE_DIS          = 0x04,
   MX25R_READ_STAT_REG      = 0x05,
   MX25R_READ_CONFIG_REG    = 0x15,
   MX25R_WRITE_STAT_REG     = 0x01,
   MX25R_SUSPEND            = 0x75,
   MX25R_RESUME             = 0x7A,
   MX25R_DEEP_SLEEP         = 0xB9,
   MX25R_SET_BURST_LEN      = 0xC0,
   MX25R_READ_ID            = 0x9F,
   MX25R_READ_EID           = 0xAB,
   MX25R_READ_EMID          = 0x90,
   MX25R_ENTER_OTP          = 0xB1,
   MX25R_EXIT_OTP           = 0xC1,
   MX25R_READ_SEC_REG       = 0x2B,
   MX25R_WRITE_SEC_REG      = 0x2F,
   MX25R_NOP                = 0x00,
   MX25R_RESET_EN           = 0x66,
   MX25R_RESET              = 0x99

} MX25RCommand;

typedef struct MX25RFLAGS {

    bool write_enabled;
    bool reset_enabled;

} MX25RFlags;

typedef struct MX25RSSTATUS {

    bool status_register_write_protected;
    bool quad_mode_enable;
    uint8_t block_protection_level;
    bool write_enabled;
    bool write_in_progress;

} MX25RStatus;

typedef struct MX25RHAL {

    /// @brief 
    uint32_t (*spi_write)(const void* const data, const uint32_t size);

    /// @brief 
    uint32_t (*spi_read)(void* const data, const uint32_t size);

    /// @brief 
    void (*select_chip)(const bool is_selected);

} MX25RHAL;

typedef struct MX25R {

    MX25RHAL hal;
    MX25RFlags flags;
    uint8_t size_in_mb;

} MX25R;

/**
 * @brief 
 * 
 * @param dev 
 * @param hal 
 * @param low_power 
 * @param size_in_mb 
 * @return MX25R* 
 */
MX25R* MX25RInit(MX25R* const dev, const MX25RHAL* const hal, const bool low_power, const uint8_t size_in_mb);

/**
 * @brief 
 * 
 * @param dev 
 */
void MX25RDeinit(MX25R* const dev);

/**
 *
 * @param address
 * @param output
 * @param size
 * @return
 */
uint8_t MX25RRead(const MX25R* const dev, const uint32_t address, uint8_t* const output, const uint32_t size);

/**
 * @brief 
 * 
 * @param dev 
 * @param address 
 * @param output 
 * @param size 
 * @return uint32_t 
 */
uint32_t MX25RFastRead(const MX25R* const dev, const uint32_t address, uint8_t* const output, const uint32_t size);

/**
 * @brief 
 * 
 * @param dev 
 * @param status 
 * @return uint8_t
 */
uint8_t MX25RReadStatus(const MX25R* const dev, MX25RStatus* const status);

/**
 * @brief 
 * 
 * @param dev 
 * @param page 
 * @param data 
 * @param size 
 * @return uint16_t 
 */
uint8_t MX25RPageProgram(const MX25R* const dev, const uint16_t page, const uint8_t* const data, const uint8_t size);

/**
 * @brief 
 * 
 * @param dev 
 * @param sector 
 * @return uint16_t 
 */
uint8_t MX25REraseSector(const MX25R* const dev, const uint16_t sector);

/**
 * @brief 
 * 
 * @param dev 
 * @param block 
 * @return uint8_t 
 */
uint8_t MX25REraseBlock32K(const MX25R* const dev, const uint8_t block);

/**
 * @brief 
 * 
 * @param dev 
 * @param block 
 * @return uint8_t 
 */
uint8_t MX25REraseBlock(const MX25R* const dev, const uint8_t block);

/**
 * @brief 
 * 
 * @param dev 
 */
uint8_t MX25REraseChip(const MX25R* const dev);

/**
 * @brief 
 * 
 * @param dev 
 */
uint8_t MX25RDeepSleep(const MX25R* const dev);

/**
 * @brief 
 * 
 * @param dev 
 * @param cmd 
 * @param args 
 * @param args_size 
 * @return uint8_t 
 */
uint8_t MX25RWriteCommand(const MX25R* const dev, const MX25RCommand cmd, const uint8_t* const args, const uint8_t args_size);

/**
 * @brief 
 * 
 * @param dev 
 * @return uint8_t 
 */
uint8_t MX25REnableWriting(MX25R* const dev);

/**
 * @brief 
 * 
 * @param dev 
 * @return uint8_t 
 */
uint8_t MX25RDisableWriting( MX25R* const dev);

/**
 * @brief 
 * 
 * @param dev 
 * @return true 
 * @return false 
 */
bool MX25RIsWritingEnabled(const MX25R* const dev);

/**
 * @brief 
 * 
 * @param dev 
 * @return true 
 * @return false 
 */
bool MX25RIsWriteInProgress(const MX25R* const dev);

#endif // include guard
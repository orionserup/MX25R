/**
 * @file MX25R.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-12-19
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "MX25R.h"

#include <string.h>

uint8_t MX25RWriteCommand(const MX25R *const dev, const MX25RCommand cmd, const uint8_t *const args, const uint8_t arg_size) {

    #ifdef DEBUG // we have to have a valid device and we can't have more than 5 args acoording to the datasheet
    if(dev == NULL || arg_size > 4)
        return 0;
    #endif

    static uint8_t buffer[5] = {0};
    buffer[0] = cmd;
    if(args != NULL)
        memcpy(buffer + 1, args, arg_size);

    return (uint8_t)dev->hal.spi_write(&cmd, 1 + arg_size);

}

#ifndef DEBUG
MX25R* MX25RInit(MX25R *const dev, const MX25RHAL* const hal, const bool low_power) {
#else
MX25R* MX25RInit(MX25R *const dev, const MX25RHAL* const hal, const bool low_power, const uint8_t size_in_mb) {

    if(hal == NULL || dev == NULL)
        return NULL;

    if(hal->select_chip == NULL || hal->spi_read == NULL || hal->spi_write == NULL)
        return NULL;
        
    dev->size_in_mb = size_in_mb;

#endif

    dev->hal = *hal;
    dev->flags = (MX25RFlags){ .write_enabled = false , .reset_enabled = false };

    const uint8_t status_config[] = {0x0, 0x0, low_power ? 0x0 : 0x02};
    dev->hal.select_chip(true);
    uint8_t res = MX25RWriteCommand(dev, MX25R_WRITE_STAT_REG, status_config, 3);
    dev->hal.select_chip(false);
    
    return res? dev: NULL;
}

void MX25RDeinit(MX25R* const dev) {

    MX25RDeepSleep(dev);

    dev->flags = (MX25RFlags){0, 0};
    dev->hal = (MX25RHAL){ NULL, NULL, NULL };
    
    #ifdef DEBUG
    dev->size_in_mb = 0;
    #endif

}

uint8_t MX25RRead(const MX25R* const dev, const uint32_t address, uint8_t *const output, const uint32_t size) {

    #ifdef DEBUG
    // if the address if bigger than the flash itself or we want to read past the end or we dont have a valid
    const uint32_t max_address = (dev->size_in_mb << 20) - 1;
    if(address > max_address || size > (max_addres - address) || output == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);

    uint8_t read_args[] = {(uint8_t)(address >> 16), (uint8_t)(address >> 8), address & 0xff};
    uint8_t ret = MX25RWriteCommand(dev, MX25R_READ, read_args, 3);
    if(ret)
        dev->hal.spi_read(output, size);

    dev->hal.select_chip(false);

    return ret;
}

uint8_t MX25RReadStatus(MX25R* const dev, MX25RStatus* const status) {

    #ifdef DEBUG
    if(dev == NULL || status == NULL)
        return 0;
    #endif

    uint8_t raw_status = 0;

    dev->hal.select_chip(true);

    uint8_t ret = MX25RWriteCommand(dev, MX25R_READ_STAT_REG, NULL, 0);
    if (ret)
        dev->hal.spi_read(&raw_status, 1);

    dev->hal.select_chip(false);

    status->write_in_progress = raw_status & 0x1;
    status->block_protection_level = (raw_status >> 2) & 0xf;
    status->write_enabled =  raw_status & 0x2;
    status->status_register_write_protected = raw_status & 0x80;
    status->quad_mode_enable = raw_status & 0x40;

    dev->flags.write_enabled = status->write_enabled;

    return ret;
}

uint8_t MX25RReadID(const MX25R* const dev, MX25RID* const id) {

    #ifdef DEBUG
    if(id == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);

    uint8_t ret = MX25RWriteCommand(dev, MX25R_READ_ID, NULL, 0);
    if(ret)
        dev->hal.spi_read(&id->id, 3);

    const uint8_t dummy_bytes[] = { 0x00, 0x00, 0x00 };
    ret = MX25RWriteCommand(dev, MX25R_READ_ESIG, dummy_bytes, 3);
    if(ret)
        dev->hal.spi_read(&id->electronic_sig, 1);

    ret = MX25RWriteCommand(dev, MX25R_READ_EMID, dummy_bytes, 3);
    if(ret)
        dev->hal.spi_read(&id->em_id, 2);

    return ret;

}

uint8_t MX25RFastRead(const MX25R* const dev, const uint32_t address, uint8_t* const output, const uint32_t size) {

    #ifdef DEBUG
    // if the address if bigger than the flash itself or we want to read past the end or we dont have a valid
    const uint32_t max_address = (dev->size_in_mb << 20) - 1;
    if(address > max_address || size > (max_addres - address) || output == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);

    uint8_t fast_read_args[] = { (uint8_t)(address >> 16), (uint8_t)(address >> 8), address & 0xff, 0 };
    uint8_t ret = MX25RWriteCommand(dev, MX25R_FAST_READ, fast_read_args, 4);
    if(ret)
        dev->hal.spi_read(output, size);

    dev->hal.select_chip(false);

    return ret;

}

uint8_t MX25RReadSecurityReg(const MX25R* const dev, MX25RSecurityReg* const reg) {

    #ifdef DEBUG
    if(reg == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);
    
    uint8_t raw_reg = 0;
    uint8_t ret = MX25RWriteCommand(dev, MX25R_READ_SEC_REG, NULL, 0);
    if(ret)
        dev->hal.spi_read(&raw_reg, 1);
    
    dev->hal.select_chip(false);

    reg->erase_failed = raw_reg & (1 << 6);
    reg->erase_suspended = raw_reg & (1 << 3);
    reg->program_failed = raw_reg & (1 << 5);
    reg->program_suspended = raw_reg & (1 << 2);
    reg->otp_sector1_locked = raw_reg & (1 << 1);
    reg->otp_sector2_locked = raw_reg & (1 << 0);

    return ret;

}

uint8_t MX25RWriteSecurityReg(const MX25R* const dev, bool lockdown_otp_sector1) {

    if(lockdown_otp_sector1)
        return MX25RWriteCommand(dev, MX25R_WRITE_SEC_REG, NULL, 0);

    return 1;
}

uint8_t MX25RWriteSecurityReg(const MX25R* const dev, bool lockdown_otp_sector1) {

    dev->hal.select_chip(true);



    dev->hal.select_chip(false);

}

uint8_t MX25RPageProgram(const MX25R* const dev, const uint16_t page, const uint8_t *const data, const uint8_t size) {

    #ifdef DEBUG
    const uint16_t max_page = (dev->size_in_mb << 20) / MX25R_PAGE_SIZE;
    if(page > max_page || data == NULL || dev->flags.write_enabled = false)
        return 0;
    #endif

    dev->hal.select_chip(true);

    uint8_t page_program_args[3] = {(uint8_t)(page >> 8), (uint8_t)(page & 0xff), 0};
    uint8_t ret = MX25RWriteCommand(dev, MX25R_PAGE_PROG, page_program_args, 3);
    uint8_t out = ret ? dev->hal.spi_write(data, size) : 0;

    dev->hal.select_chip(false);

    return out;
}

uint8_t MX25REraseSector(const MX25R* const dev, const uint16_t sector) {

    #ifdef DEBUG
    const uint16_t max_sector = (dev->size_in_mb << 20) / MX25R_SECTOR_SIZE;
    if(sector > max_sector || dev->flags.write_enabled == false)
        return 0;
    #endif

    const uint8_t erase_sector_args[] = { 0, (sector >> 8), sector & 0xff };

    dev->hal.select_chip(true);
    uint8_t ret = MX25RWriteCommand(dev, MX25R_SECT_ERASE, erase_sector_args, 3);
    dev->hal.select_chip(false);
    
    return ret;

}

uint8_t MX25REraseBlock32K(const MX25R* const dev, const uint8_t block) {

    #ifdef DEBUG
    const uint8_t max_small_block = (dev->size_in_mb << 20) / MX25R_SMALL_BLOCK_SIZE;
    if(block > max_small_block || dev->flags.write_enabled == false)
        return 0;
    #endif

    const uint8_t erase_block_args[] = { 0, 0, block };

    dev->hal.select_chip(true);
    uint8_t ret = MX25RWriteCommand(dev, MX25R_BLOCK_ERASE32K, erase_block_args, 3);
    dev->hal.select_chip(false);

    return ret;
}

uint8_t MX25REraseBlock(const MX25R* const dev, const uint8_t block) {

    #ifdef DEBUG
    const uint8_t max_block = (dev->size_in_mb << 20) / MX25R_BLOCK_SIZE;
    if(block > max_block || dev->flags.write_enabled == false)
        return 0;
    #endif

    const uint8_t erase_block_args[] = { 0, 0, block };

    dev->hal.select_chip(true);
    uint8_t ret = MX25RWriteCommand(dev, MX25R_BLOCK_ERASE, erase_block_args, 3);
    dev->hal.select_chip(false);

    return ret;

}

uint8_t MX25REraseChip(const MX25R* const dev) {

    #ifdef DEBUG
    if(dev == NULL || dev->flags.write_enabled == false)
        return 0;
    #endif

    dev->hal.select_chip(true);
    uint8_t res = MX25RWriteCommand(dev, MX25R_CHIP_ERASE, NULL, 0);
    dev->hal.select_chip(false);


    return res;

}

uint8_t MX25RDeepSleep(const MX25R* const dev) {

    #ifdef DEBUG
    if(dev == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);
    uint8_t res = MX25RWriteCommand(dev, MX25R_DEEP_SLEEP, NULL, 0);
    dev->hal.select_chip(false);
    
    return res;

}

uint8_t MX25REnableWriting(MX25R* const dev)  {

    dev->hal.select_chip(true);
    uint8_t ret = MX25RWriteCommand(dev, MX25R_WRITE_EN, NULL, 0);
    dev->hal.select_chip(false);

    dev->flags.write_enabled = true;
    return ret;
}

uint8_t MX25RDisableWriting(MX25R* const dev)  {

    dev->hal.select_chip(true);
    uint8_t ret = MX25RWriteCommand(dev, MX25R_WRITE_DIS, NULL, 0);
    dev->hal.select_chip(false);

    dev->flags.write_enabled = false;
    return ret;
}

bool MX25RIsWritingEnabled(const MX25R* const dev) { return dev->flags.write_enabled; }

bool MX25RIsWriteInProgress(MX25R* const dev) {
    
    MX25RStatus stat = {0};
    MX25RReadStatus(dev, &stat);
    return stat.write_in_progress;

}

bool MX25RVerifyErase(const MX25R* const dev)  {

    MX25RSecurityReg reg;
    MX25RReadSecurityReg(dev, &reg);
    return !(reg.erase_failed || reg.erase_suspended);

}

bool MX25RVerifyProgram(const MX25R* const dev) {

    MX25RSecurityReg reg;
    MX25RReadSecurityReg(dev, &reg);
    return !(reg.program_failed || reg.program_suspended);

}

uint8_t MX25RSetBurstLength(const MX25R* const dev);
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

MX25R* MX25RInit(MX25R *const dev, const MX25RHAL* const hal, const bool low_power, const uint8_t size_in_mb) {

    #ifdef DEBUG // checking for error cases 

    if(hal == NULL || dev == NULL)
        return NULL;

    if(hal->select_chip == NULL || hal->spi_read == NULL || hal->spi_write == NULL)
        return NULL;

    #endif

    dev->hal = *hal;
    dev->size_in_mb = size_in_mb;
    dev->flags = (MX25RFlags){ .write_enabled = false , .reset_enabled = false };

    const uint8_t status_config[] = {0x2, 0x0, low_power ? 0x0 : 0x02};
    dev->hal.select_chip(true);
    MX25RWriteCommand(dev, MX25R_WRITE_STAT_REG, status_config, 3);
    dev->hal.select_chip(false);

    return dev;
}

void MX25RDeinit(MX25R* const dev) {

    MX25RDeepSleep(dev);
}

uint8_t MX25RRead(const MX25R* const dev, const uint32_t address, uint8_t *const output, const uint32_t size){

    #ifdef DEBUG
    // if the address if bigger than the flash itself or we want to read past the end or we dont have a valid
    const uint32_t max_address = (dev->size_in_mb << 20) - 1;
    if(address > max_address || size > (max_addres - address) || output == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);

    uint8_t address_buffer[] = {(uint8_t)(address >> 16), (uint8_t)(address >> 8), address & 0xff};
    uint8_t ret = MX25RWriteCommand(dev, MX25R_READ, address_buffer, 3);
    uint8_t out = ret == 0 ? 0 : (uint8_t)dev->hal.spi_read(output, size);

    dev->hal.select_chip(false);

    return ret ? (uint8_t)out : 0;
}

uint8_t MX25RReadStatus(const MX25R* const dev, MX25RStatus* const status) {

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

    return ret;
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

uint8_t MX25REraseChip(const MX25R* const dev){

    #ifdef DEBUG
    if(dev == NULL || dev->flags.write_enabled == false)
        return 0;
    #endif

    dev->hal.select_chip(true);
    uint8_t res = MX25RWriteCommand(dev, MX25R_CHIP_ERASE, NULL, 0);
    dev->hal.select_chip(false);

    return res;

}

uint8_t MX25RDeepSleep(MX25R* const dev) {

    #ifdef DEBUG
    if(dev == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);
    uint8_t res = MX25RWriteCommand(dev, MX25R_DEEP_SLEEP, NULL, 0);
    dev->hal.select_chip(false);
    
    return res;

}

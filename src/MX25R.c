/**
 * @file MX25R.c
 * @author Orion Serup (oserup@proton.me)
 * @brief Contains the Implementation of the MX25R Functionality
 * @version 0.1
 * @date 2022-12-19
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "MX25R.h"

#include <string.h>

/**
 * @brief Actually sends the command to be executed with the parameters, selects the device, writes the command and unselects the device so it executes
 * 
 * @param[in] dev: Device to execute the command on 
 * @param[in] command: Command to Execute 
 * @param[in] args: Arguments for the command 
 * @param[in] args_size: How many arguments 
 * @return uint8_t: Command Status, 0 if there was an error 
 */
static uint8_t MX25RExecComplexCommand(const MX25R* const dev, const MX25RCommand command, const uint8_t* const args, const uint8_t args_size) {

    #ifdef DEBUG
    if(dev == NULL || args == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);
    uint8_t out = MX25RWriteCommand(dev, command, args, args_size);
    dev->hal.select_chip(false);

    return out;

}

/**
 * @brief Executes a Command that takes no parameters
 * 
 * @param[in] dev: Device to Execute the command on 
 * @param[in] command: Command to Execute 
 * @return uint8_t: Command status  
 */
static uint8_t MX25RExecSimpleCommand(const MX25R* const dev, const MX25RCommand command) { return MX25RExecComplexCommand(dev, command, NULL, 0); }

/**
 * @brief Executes a command which writes some buffer to the device for whatever reason
 * 
 * @param[in] dev: Device to execute the command on
 * @param[in] command: Command to execute 
 * @param[in] args: Arguments for that command 
 * @param[in] args_size: How many arguments the command takes 
 * @param[in] buffer: Buffer to write from 
 * @param[in] size: How many bytes to write 
 * @return uint8_t: The Command Execution status, 0 if there was an error 
 */
static uint8_t MX25RExecWritingCommand(const MX25R* const dev, const MX25RCommand command, const uint8_t* const args, const uint8_t args_size, const void* const buffer, const uint8_t size) {

    #ifdef DEBUG
    if(dev == NULL || buffer == NULL || size == 0 || dev->is_write_en == false)
        return 0;
    #endif

    dev->hal.select_chip(true);

    uint8_t ret = MX25RWriteCommand(dev, command, args, args_size);
    
    if(ret)
        dev->hal.spi_write(buffer, size);

    dev->hal.select_chip(false);

    return ret;

}

/**
 * @brief Fully Implements the Procedure of an Erase Type command
 * 
 * @param[in] dev: Device to Erase data of 
 * @param[in] cmd: Erase Type command 
 * @param[in] args: Arguments for that command 
 * @param[in] args_size: The number of arguments for that command 
 * @return uint8_t: The command status, 0 if there was an error 
 */
static uint8_t MX25RExecEraseCommand(const MX25R* const dev, const MX25RCommand cmd, const uint8_t* const args, const uint8_t args_size) {

    #ifdef DEBUG
    if(dev->is_write_en == false)
        return 0;
    #endif

    return MX25RExecComplexCommand(dev, cmd, args, args_size);

}

/**
 * @brief Fully Implements a reading command, ie it selects the chip, writes the command to the ship, and then reads the values and unselects the chip
 * 
 * @param[in] dev: Device to read from 
 * @param[in] cmd: Reading Command 
 * @param[in] args: Arguments for the reading command 
 * @param[in] args_size: How many argumments are for that command 
 * @param[out] out: The Buffer to write the read information into 
 * @param[in] size: How many bytes to read  
 * @return uint8_t: The command state, 0 if there was an error 
 */
static uint8_t MX25RExecReadingCommand(const MX25R* const dev, const MX25RCommand cmd, const uint8_t* args, const uint8_t args_size, void* const out, const uint32_t size) {

    #ifdef DEBUG
    if(dev == NULL)
        return 0;
    #endif

    dev->hal.select_chip(true);

    uint8_t ret = MX25RWriteCommand(dev, cmd, args, args_size);
    
    if(ret)
        dev->hal.spi_read(out, size);

    dev->hal.select_chip(false);

    return ret;

}

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
    dev->is_write_en = false;

    const uint8_t status_config[] = {0x0, 0x0, low_power ? 0x0 : 0x02};
    uint8_t res = MX25RExecComplexCommand(dev, MX25R_WRITE_STAT_REG, status_config, 3);
    
    return res? dev: NULL;
}

void MX25RDeinit(MX25R* const dev) {

    MX25RDeepSleep(dev);

    dev->is_write_en = false;
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

    uint8_t read_args[] = {(uint8_t)(address >> 16), (uint8_t)(address >> 8), address & 0xff};
    return MX25RExecReadingCommand(dev, MX25R_READ, read_args, 3, output, size);
}

uint8_t MX25RReadStatus(MX25R* const dev, MX25RStatus* const status) {

    #ifdef DEBUG
    if(dev == NULL || status == NULL)
        return 0;
    #endif

    uint8_t raw_status = 0;

    uint8_t ret = MX25RExecReadingCommand(dev, MX25R_READ_STAT_REG, NULL, 0, &raw_status, 1);

    status->write_in_progress = raw_status & (1 << 0);
    status->block_protection_level = (raw_status >> 2) & 0xf;
    status->write_enabled =  raw_status & (1 << 1);
    status->status_register_write_protected = raw_status & (1 << 7);
    status->quad_mode_enable = raw_status & (1 << 2);

    dev->is_write_en = status->write_enabled;

    return ret;
}

uint8_t MX25RReadID(const MX25R* const dev, MX25RID* const id) {

    #ifdef DEBUG
    if(id == NULL)
        return 0;
    #endif

    const uint8_t dummy_bytes[] = { 0x00, 0x00, 0x00 };
    
    uint8_t ret = MX25RExecReadCommand(dev, MX25R_READ_ID, NULL, 0, &id->id, 3);
    ret *= MX25RExecuteReadCommand(dev, MX25R_READ_ESIG, dummy_bytes, 3, &id->electronic_sig, 1);
    ret *= MX25RExecuteReadCommand(dev, MX25R_READ_EMID, dummy_bytes, 3, &id->em_id, 2);

    return ret;

}

uint8_t MX25RFastRead(const MX25R* const dev, const uint32_t address, uint8_t* const output, const uint32_t size) {

    #ifdef DEBUG
    // if the address if bigger than the flash itself or we want to read past the end or we dont have a valid
    const uint32_t max_address = (dev->size_in_mb << 20) - 1;
    if(address > max_address || size > (max_addres - address) || output == NULL)
        return 0;
    #endif

    uint8_t fast_read_args[] = { (uint8_t)(address >> 16), (uint8_t)(address >> 8), address & 0xff, 0 };
    return MX25RExecReadingCommand(dev, MX25R_FAST_READ, fast_read_args, 3, output, size);

}

uint8_t MX25RReadSecurityReg(const MX25R* const dev, MX25RSecurityReg* const reg) {

    #ifdef DEBUG
    if(reg == NULL)
        return 0;
    #endif
    
    uint8_t raw_reg = 0;
    uint8_t ret = MX25RExecReadingCommand(dev, MX25R_READ_SEC_REG, NULL, 0, &raw_reg, 1);

    reg->erase_failed = raw_reg & (1 << 6);
    reg->erase_suspended = raw_reg & (1 << 3);
    reg->program_failed = raw_reg & (1 << 5);
    reg->program_suspended = raw_reg & (1 << 2);
    reg->otp_sector1_locked = raw_reg & (1 << 1);
    reg->otp_sector2_locked = raw_reg & (1 << 0);

    return ret;

}

uint8_t MX25RWriteSecurityReg(const MX25R* const dev, bool lockdown_otp_sector1) {

    uint8_t res = 1;
    if(lockdown_otp_sector1) 
        res = MX25RExecSimpleCommand(dev, MX25R_WRITE_SEC_REG);
    
    return res;
}

uint8_t MX25RPageProgram(const MX25R* const dev, const uint16_t page, const uint8_t *const data, const uint8_t size) {

    #ifdef DEBUG
    const uint16_t max_page = (dev->size_in_mb << 20) / MX25R_PAGE_SIZE;
    if(page > max_page || data == NULL || dev->flags.write_enabled = false)
        return 0;
    #endif

    uint8_t page_program_args[3] = {(uint8_t)(page >> 8), (uint8_t)(page & 0xff), 0};
    return MX25RExecWritingCommand(dev, MX25R_PAGE_PROG, page_program_args, 3, data, size);
}

uint8_t MX25REraseSector(const MX25R* const dev, const uint16_t sector) {

    #ifdef DEBUG
    const uint16_t max_sector = (dev->size_in_mb << 20) / MX25R_SECTOR_SIZE;
    if(sector > max_sector)
        return 0;
    #endif

    const uint8_t erase_sector_args[] = { 0, (sector >> 8), sector & 0xff };
    return MX25RExecEraseCommand(dev, MX25R_SECT_ERASE, erase_sector_args, 3);

}

uint8_t MX25REraseBlock32K(const MX25R* const dev, const uint8_t block) {

    #ifdef DEBUG
    const uint8_t max_small_block = (dev->size_in_mb << 20) / MX25R_SMALL_BLOCK_SIZE;
    if(block > max_small_block)
        return 0;
    #endif

    const uint8_t erase_block_args[] = { 0, 0, block };
    return MX25RExecEraseCommand(dev, MX25R_BLOCK_ERASE32K, erase_block_args, 3);
}

uint8_t MX25REraseBlock(const MX25R* const dev, const uint8_t block) {

    #ifdef DEBUG
    const uint8_t max_block = (dev->size_in_mb << 20) / MX25R_BLOCK_SIZE;
    if(block > max_block)
        return 0;
    #endif

    const uint8_t erase_block_args[] = { 0, 0, block };
    return MX25RExecEraseCommand(dev, MX25R_BLOCK_ERASE, erase_block_args, 3);

}

uint8_t MX25REnableBurstRead(const MX25R* const dev, const uint8_t wrap_length) {

    #ifdef DEBUG 
    if(wrap_length > 3)
        return 0;
    #endif

    return MX25RExecComplexCommand(dev, MX25R_SET_BURST_LEN, &wrap_length, 1);

}

uint8_t MX25RDisableBurstRead(const MX25R* const dev) {

    static const uint8_t burst_disable = 0x10;
    return MX25RExecComplexCommand(dev, MX25R_SET_BURST_LEN, &burst_disable, 1);

}

uint8_t MX25REraseChip(const MX25R* const dev) { return MX25RExecEraseCommand(dev, MX25R_FLASH_ERASE, NULL, 0); }

uint8_t MX25RDeepSleep(const MX25R* const dev) { return MX25RExecSimpleCommand(dev, MX25R_DEEP_SLEEP); }

uint8_t MX25REnableWriting(MX25R* const dev)  {

    dev->is_write_en = true;
    return MX25RExecSimpleCommand(dev, MX25R_WRITE_EN);
}

uint8_t MX25RDisableWriting(MX25R* const dev)  {

    dev->is_write_en = false;
    return MX25RExecSimpleCommand(dev, MX25R_WRITE_DIS);
}

bool MX25RIsWritingEnabled(const MX25R* const dev) { return dev->is_write_en; }

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

uint8_t MX25RReset(MX25R* const dev) { 

    dev->is_write_en = false;   
    return MX25RExecSimpleCommand(dev, MX25R_RESET_EN) && MX25RExecSimpleCommand(dev, MX25R_RESET); 
    
}

bool MX25RIsOTPRegionLocked(const MX25R* const dev) { 
    
    MX25RSecurityReg reg = {0};
    MX25RReadSecurityReg(dev, &reg);
    return reg.otp_sector1_locked && reg.otp_sector2_locked;
    
}

uint8_t MX25REnterOTPRegion(const MX25R* const dev) { return MX25RExecSimpleCommand(dev, MX25R_ENTER_OTP); }

uint8_t MX25RExitOTPRegion(const MX25R* const dev)  { return MX25RExecSimpleCommand(dev, MX25R_EXIT_OTP); }

uint8_t MX25RSuspend(const MX25R* const dev) { return MX25RExecSimpleCommand(dev, MX25R_SUSPEND); }

uint8_t MX25RResume(const MX25R* const dev) { return MX25RExecSimpleCommand(dev, MX25R_RESUME); }

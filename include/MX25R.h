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

#define MX25R_PAGE_SIZE         256     ///< 2 ^ 8, How large each code page is
#define MX25R_SECTOR_SIZE       4096    ///< 2 ^ 12, How Large Each Sector is 
#define MX25R_SMALL_BLOCK_SIZE  32768   ///< 2 ^ 15, How Large Each half Block is
#define MX25R_BLOCK_SIZE        65536   ///< 2 ^ 16, How large the Full Block is

/// @brief All of the commands that can be run on the flash
typedef enum MX25RCOMMAND {

   MX25R_READ               = 0x03, ///< Read a series of bytes from flash
   MX25R_FAST_READ          = 0x0B, ///< Read a series of bytes but faster
   MX25R_DOUBLE_READ        = 0xBB, ///< Two Channel Input and Output Reading  
   MX25R_DREAD              = 0x3B, ///< One Channel Input 2 Channel Output Reading
   MX25R_QUAD_READ          = 0xEB, ///< Four Channel Input and Output Reading
   MX25R_QREAD              = 0x6B, ///< One Channel Input and Four Channel Output 
   MX25R_PAGE_PROG          = 0x02, ///< Program a 256 byte page that was erased
   MX25R_QPAGE_PROG         = 0x38, ///< Quad Input/Output program a page
   MX25R_SECT_ERASE         = 0x20, ///< Erase a 4KB Sector so that it can be programmed
   MX25R_BLOCK_ERASE32K     = 0x52, ///< Erase a 32KB Block so that it can be programmed
   MX25R_BLOCK_ERASE        = 0xD8, ///< Erase a 64KB Block so that it can be programmed
   MX25R_CHIP_ERASE         = 0x60, ///< Erase the Entire Chip
   MX25R_FLASH_ERASE        = 0xC7, ///< Erase all of the Flash Memory
   MX25R_READ_SFDP          = 0x5A, ///< Read the SFDP(Serial Flash Discoverable Parameter) Register
   MX25R_WRITE_EN           = 0x06, ///< Enable Erasing and Programming the Flash
   MX25R_WRITE_DIS          = 0x04, ///< Disable Erasinfg and Programming the Flash   
   MX25R_READ_STAT_REG      = 0x05, ///< Read the Status Register
   MX25R_READ_CONFIG_REG    = 0x15, ///< Read the Configuration Register
   MX25R_WRITE_STAT_REG     = 0x01, ///< Write to the Status and Configuration Register Together
   MX25R_SUSPEND            = 0x75, ///< Suspend any Erase or Programming Actions
   MX25R_RESUME             = 0x7A, ///< Resume any suspended Action
   MX25R_DEEP_SLEEP         = 0xB9, ///< Put the Flash in to ultra low power deep sleep
   MX25R_SET_BURST_LEN      = 0xC0, ///< Set the read burst length for burst reading
   MX25R_READ_ID            = 0x9F, ///< Read the Chip ID
   MX25R_READ_ESIG          = 0xAB, ///< Read the Electrical Signature
   MX25R_READ_EMID          = 0x90, ///< Read the Electromechanical ID
   MX25R_ENTER_OTP          = 0xB1, ///< Enter the OTP Region of the Flash
   MX25R_EXIT_OTP           = 0xC1, ///< Leave the OTP Region of the Flash
   MX25R_READ_SEC_REG       = 0x2B, ///< Read the Security Status and Features Register 
   MX25R_WRITE_SEC_REG      = 0x2F, ///< Write the Security Status and Features Register
   MX25R_NOP                = 0x00, ///< Does Nothing
   MX25R_RESET_EN           = 0x66, ///< Enables Software Reset
   MX25R_RESET              = 0x99  ///< Software Reset

} MX25RCommand;

/// @brief Runtime flags that are associated with Hardware ability/config
typedef struct MX25RFLAGS {

    bool write_enabled;     ///< If the Flash is writable
    bool reset_enabled;     ///< If Software Reset is enabled

} MX25RFlags;

/// @brief All of the flags from the Security Register
typedef struct MX25RSECURITYREG {

    bool erase_failed;      ///< If the Erase that was executed failed
    bool program_failed;    ///< If the Program that was executed failed
    bool erase_suspended;   ///< If there is an Erase operation in progress that was suspended
    bool program_suspended; ///< If there was a Program operation in progress that was suspended
    bool otp_sector1_locked;///< If the first sector of the OTP section is locked for writing
    bool otp_sector2_locked;///< If the second sector of the OTP section is locked, will be factory programmed

} MX25RSecurityReg;

/// @brief Represents the status of the base operations
typedef struct MX25RSSTATUS {

    bool status_register_write_protected;   ///< If this register can be written to
    bool quad_mode_enable;                  ///< If Quad Channel I/O is Enabled
    uint8_t block_protection_level;         ///< How many blocks are set to be protected via block protection (2 ^ value) either from the top or from the bottom (default top)
    bool write_enabled;                     ///< If we are allowed to write to flash
    bool write_in_progress;                 ///< If there is a current program or erase operation in progress

} MX25RStatus;

#pragma pack(push, 1)

/// @brief All of the Device Identification Information
typedef struct MX25RID {

    struct { 
        uint8_t man_id;     ///< Manufacturer ID (Should be 0xC2)
        uint8_t mem_type;   ///< Memory Type (Should be 0x28)
        uint8_t mem_density;///< Memory Density (Depends on Flash Size)
    } id;
    
    uint8_t electronic_sig;        ///< Electronic ID (Should be 0x15)
    struct { 
        uint8_t man_id;     ///< Manufacturer ID (Should be 0xC2)
        uint8_t dev_id;     ///< Device ID (Depends on Device) 
    } em_id;

} MX25RID;

#pragma pack(pop)

/// @brief Contains all of the Hardware functions needed to communicate with the flash, primarily SPI
typedef struct MX25RHAL {

    /// @brief Writes to a Device on the SPI Bus, returns the number of bytes written, 0 if there was an error
    uint32_t (*spi_write)(const void* const data, const uint32_t size); 

    /// @brief Reads from a device on the SPI bus, returns the number of bytes read, 0 if there was an error
    uint32_t (*spi_read)(void* const data, const uint32_t size);

    /// @brief Toggles the CS pin, true corresponds the to pin being pulled low and vice versa
    void (*select_chip)(const bool is_selected);

} MX25RHAL;

/// @brief A struct representing the flash device
typedef struct MX25R {

    MX25RHAL hal;       ///< Hardware functions to control the Flash
    MX25RFlags flags;   ///< Configuration and other runtime flags
    #ifdef DEBUG
    uint8_t size_in_mb; ///< How big the flash is in megabytes, used for bound checking ( only in debug )
    #endif
} MX25R;

// -------------------------------------- Init and Deinit ------------------------------------ //

#ifdef DEBUG

/**
 * @brief Initializes a MX25R Object with the given parameters
 * 
 * @param[out] dev: Device to Initialize
 * @param[in] hal: Hardware functionality to give it
 * @param[in] low_power: If we want it running in low power mode
 * @param[in] size_in_mb: How big in MB the flash is, used for bounds checking
 * @return MX25R*: NULL if it failed to initialize and dev if it worked
 */
MX25R* MX25RInit(MX25R* const dev, const MX25RHAL* const hal, const bool low_power, const uint8_t size_in_mb);

#else

/**
 * @brief Initializes the MX25R Object
 * 
 * @param[in] dev: Device we want to Initialize 
 * @param[in] hal: Hardware level functions for the device
 * @param[in] low_power: If we want the device to be in low power or performance mode
 * @return MX25R*: The pointer to the initialized object, NULL if init failed
 */
MX25R* MX25RInit(MX25R* const dev, const MX25RHAL* const hal, const bool low_power);

#endif

/**
 * @brief Deinitializes a flash object
 * 
 * @param[in] dev: Flash to Deinit 
 */
void MX25RDeinit(MX25R* const dev);

// -------------------------------------------- Reading Functions ---------------------------------- //

/**
 * @brief Reads bytes from flash
 * 
 * @param[in] dev: Device to read from 
 * @param[in] address: Address to read from 
 * @param[out] output: Buffer to read into
 * @param[in] size: How many bytes to read 
 * @return uint8_t: How many bytes were processed in the command, 0 if there was an error 
 */
uint8_t MX25RRead(const MX25R* const dev, const uint32_t address, uint8_t* const output, const uint32_t size);

/**
 * @brief Reads from the flash at max speed
 * 
 * @param[in] dev: Device to read from 
 * @param[in] address: Address to read from 
 * @param[out] output: Buffer to read into 
 * @param[in] size: How many bytes to read 
 * @return uint8_t: How many bytes were processed in the command, 0 if error 
 */
uint8_t MX25RFastRead(const MX25R* const dev, const uint32_t address, uint8_t* const output, const uint32_t size);

/**
 * @brief Reads the Status Register into a Status Object
 * 
 * @param[in] dev: Device to read from
 * @param[out] status: Status buffer to read into 
 * @return uint8_t: How many bytes were processed by the command, 0 if there was an error
 */
uint8_t MX25RReadStatus(MX25R* const dev, MX25RStatus* const status);

/**
 * @brief Reads the security register from the device, contains past state information and if the OTP is writable
 * 
 * @param[in] dev: Device to read from 
 * @param[out] reg: Where to read the register to
 * @return uint8_t: The Command execution status, 0 if there was an error 
 */
uint8_t MX25RReadSecurityReg(const MX25R* const dev, MX25RSecurityReg* const reg);

/**
 * @brief Read all of the ID 
 * 
 * @param[in] dev: Device to read the ID from 
 * @param[out] id: ID struct to write into 
 * @return uint8_t: Command Execution status, 0 if failure 
 */
uint8_t MX25RReadID(const MX25R* const dev, MX25RID* const id);

// ------------------------------------ Writing and Programming Functions ------------------------------ //

/**
 * @brief Write to the Security register, the only configurable paramaters is an OTP flag to lock out the first part of the OTP block
 * 
 * @param[in] dev: Device we want to write to
 * @param[in] lockdown_otp_sector1: If we want to lock down the first sector of the OTP region
 * @return uint8_t: Command 
 */
uint8_t MX25RWriteSecurityReg(const MX25R* const dev, bool lockdown_otp_sector1);

/**
 * @brief Programs a page (256 bytes) with whatever you feed it
 * @note Page must be erased before programming
 * @param[in] dev: Device to write to
 * @param[in] page: Which page to write to 
 * @param[in] data: Data to Write to the page 
 * @param[in] size: How many bytes to write to the page - 1, 0 is 1 byte written 
 * @return uint8_t: How many bytes were registered with the command, 0 if there was an error  
 */
uint8_t MX25RPageProgram(const MX25R* const dev, const uint16_t page, const uint8_t* const data, const uint8_t size);

// ----------------------------------------- Erasing Functions ----------------------------------------------- //

/**
 * @brief Erases a Sector of size 4096 so that it can be reprogrammed
 * 
 * @param[in] dev: Device to erase the sector of
 * @param[in] sector: Sector to erase 
 * @return uint8_t: How many bytes of the command were processed successfully, 0 if there was an error  
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
 * @return uint8_t 
 */
uint8_t MX25REraseChip(const MX25R* const dev);

// ---------------------------------------- OTP Functions --------------------------------------- //

/**
 * @brief Enters the OTP region so you can program the OTP Region
 * 
 * @param dev 
 * @return uint8_t 
 */
uint8_t MX25REnterOTPRegion(const MX25R* const dev);

/**
 * @brief 
 * 
 * @param dev 
 * @return uint8_t 
 */
uint8_t MX25RExitOTPRegion(const MX25R* const dev);

/**
 * @brief 
 * 
 * @param dev 
 * @return true 
 * @return false 
 */
bool MX25RIsOTPRegionLocked(const MX25R* const dev);

// ------------------------------------- Utility Functions ------------------------------------ //

/**
 * @brief Verifies if an Erase was performed fully
 * 
 * @param[in] dev: Device to Check 
 * @return true: If the Erase was successful
 * @return false: If the Erase Failed
 */
bool MX25RVerifyErase(const MX25R* const dev);

/**
 * @brief Verifies if a Program Performed correctly
 * 
 * @param[in] dev: Device to verify 
 * @return true: If the program was successful
 * @return false: If the program failed 
 */
bool MX25RVerifyProgram(const MX25R* const dev);

/**
 * @brief Puts the device into Deep Sleep, super low power option
 * 
 * @param[in] dev: Device to put into deep sleep 
 * @return uint8_t: Command Execution status, 0 if there is an error 
 */
uint8_t MX25RDeepSleep(const MX25R* const dev);

/**
 * @brief Enables high speed burst reading with wrap around of a certain length
 * 
 * @param[in] dev: Device to Enable Burst reading on 
 * @param[in] wrap_length: 0 - 3, 0: 8 bytes, 1: 16 bytes, 2: 32 bytes, 3: 64 bytes, else disabled 
 * @return uint8_t: Command Execution status, 0 if there was an error
 */
uint8_t MX25REnableBurstRead(const MX25R* const dev, const uint8_t wrap_length);

/**
 * @brief Disables burst reading with wrap around
 * 
 * @param[in] dev: Device to disable burst reading on
 * @return uint8_t: Command Execution Status, 0 if error
 */
uint8_t MX25RDisableBurstRead(const MX25R* const dev);

// --------------------------------- State Setting and Reading Functions ----------------------------- //

/**
 * @brief Enables erasing and programming of the device
 * 
 * @param[in] dev 
 * @return uint8_t 
 */
uint8_t MX25REnableWriting(MX25R* const dev);

/**
 * @brief 
 * 
 * @param[in] dev 
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
bool MX25RIsWriteInProgress(MX25R* const dev);

// --------------------------------------------- Low Level Exposed API ---------------------------------------------- //

/**
 * @brief Writes a command packet to the device, each command takes its own args
 * 
 * @param[in] dev: Device to write to 
 * @param[in] cmd: Command to Send 
 * @param[in] args: Arguments to that command, NULL if there are none 
 * @param[in] args_size: How many arguments are for the comamand 
 * @return uint8_t: The status of the transfer, 0 if there was an error 
 */
uint8_t MX25RWriteCommand(const MX25R* const dev, const MX25RCommand cmd, const uint8_t* const args, const uint8_t args_size);

#endif // include guard
#ifndef BOOTLOADER_SRC_JUMPCODE_H_
#define BOOTLOADER_SRC_JUMPCODE_H_

#include <stdint.h>

typedef enum
{
  AUTO,
  BOOT
} JumpCode_t;

/* Software Reset */
/* Rebase the stack pointer and the vector table base address to bootloader */
#define RESET_CMD() __set_MSP(*(uint32_t *) (ADDR_FLASH_SECTOR_0));  \
  SCB->VTOR = ((uint32_t) (ADDR_FLASH_SECTOR_0) & SCB_VTOR_TBLOFF_Msk); \
    NVIC_SystemReset()

void clearJumpCode();
void setJumpCode(JumpCode_t code, uint8_t arg);
void getJumpCode(JumpCode_t * code, uint8_t * arg);

#endif /* BOOTLOADER_SRC_JUMPCODE_H_ */

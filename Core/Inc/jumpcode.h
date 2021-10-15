#ifndef BOOTLOADER_SRC_JUMPCODE_H_
#define BOOTLOADER_SRC_JUMPCODE_H_

#include <stdint.h>

typedef enum
{
  AUTO,
  BOOT
} JumpCode_t;

void clearJumpCode();
void setJumpCode(JumpCode_t code, uint8_t arg);
void getJumpCode(JumpCode_t * code, uint8_t * arg);

#endif /* BOOTLOADER_SRC_JUMPCODE_H_ */

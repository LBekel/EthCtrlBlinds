#include <stdint.h>
#include <string.h>

#include "jumpcode.h"
#include "stm32f7xx_hal.h"

#define BOOTLOADER_JUMPCODES_COUNT 2
#define BOOTLOADER_JUMPCODE_LENGTH 4

#define BOOTLOADER_JUMPCODE_SIZE 16
volatile unsigned char __attribute__((section (".bl_interface_buffer")))
                  jumpcode[BOOTLOADER_JUMPCODE_SIZE];

static char const jumpcodeSTR[BOOTLOADER_JUMPCODES_COUNT][BOOTLOADER_JUMPCODE_LENGTH] = {
                                     "AUTO",
                                     "BOOT"
};

void clearJumpCode()
{
  for(uint8_t i = 0; i < BOOTLOADER_JUMPCODE_LENGTH + 2; i++)
  {
    jumpcode[i] = 0;
  }
}

void setJumpCode(JumpCode_t code, uint8_t arg)
{
  //disable the data cache. Otherwise the data we write now
  //wont be available after reset
  //SCB_DisableDCache();

  uint8_t parity = 0x1f; //preload
  uint8_t i = 0;

  for(; i < BOOTLOADER_JUMPCODE_LENGTH; i++)
  {
    parity ^= jumpcodeSTR[code][i];
    jumpcode[i] = jumpcodeSTR[code][i];
  }
  parity ^= arg;
  jumpcode[i++] = arg;
  jumpcode[i] = parity;
}

void getJumpCode(JumpCode_t * code, uint8_t * arg)
{
  uint8_t parity = 0x1f; //preload
  uint8_t i = 0;

  for(; i < BOOTLOADER_JUMPCODE_LENGTH; i++)
  {
    parity ^= jumpcode[i];
  }
  parity ^= jumpcode[i++];
  if(parity != jumpcode[i])
  {
    //invalid jumpcode
    //use default
    *code = AUTO;
    *arg = 0;
    return;
  }
  for(i = 0; i < BOOTLOADER_JUMPCODES_COUNT; i++)
  {
    if(memcmp((const unsigned char *)jumpcode, jumpcodeSTR[i], 4) == 0)
    {
      *code = (JumpCode_t)i;
      *arg = jumpcode[BOOTLOADER_JUMPCODE_LENGTH];
      return;
    }
  }
  //unknown jumpcode
  //use default
  *code = AUTO;
  *arg = 0;
}

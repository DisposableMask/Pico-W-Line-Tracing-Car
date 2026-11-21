#ifndef ROVER_FLASH_H
#define ROVER_FLASH_H

int rover_flash_execute(void (*operation)(void *), void *arg);

#endif

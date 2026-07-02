#ifndef APP_KEYS_H
#define APP_KEYS_H

#include <stdint.h>

typedef enum
{
    KEY_NONE = 0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_1_2
} KeyEvent;

KeyEvent Keys_Scan(void);

#endif

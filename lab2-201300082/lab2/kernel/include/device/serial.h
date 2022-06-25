#ifndef __SERIAL_H__
#define __SERIAL_H__

#define MAX_INPUT_SIZE 200
typedef struct
{
    int size;
    char buf[MAX_INPUT_SIZE];
} InputBuf;
extern InputBuf inputBuf;
void clearBuf(InputBuf *buf);

void initSerial(void);
void putChar(char);
void log(const char *str);
#define SERIAL_PORT  0x3F8

#endif

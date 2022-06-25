#include "x86.h"
#include "device.h"

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;


void GProtectFaultHandle(struct TrapFrame *tf);

void KeyboardHandle(struct TrapFrame *tf);

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallGetChar(struct TrapFrame *tf);
void syscallGetStr(struct TrapFrame *tf);
void timerHandle(struct TrapFrame *tf) {
	return;
}
void printChar(char c);
InputBuf inputBuf;
void clearBuf(InputBuf *buf)
{
	buf->size = 0;
}
void insertBuf(InputBuf *buf, char c)
{
	buf->buf[buf->size++] = c;
	if (buf->size > MAX_INPUT_SIZE)
	{
		assert(0);
	}
}
int deleteBack(InputBuf *buf)
{
	if (buf->size)
	{
		--buf->size;
		return 1;
	}
	return 0;
}

char getbackChar(InputBuf *buf)
{
	// getback a single char
	asm volatile("sti");
	while (!buf->size || buf->buf[buf->size - 1] != '\n')
		waitForInterrupt();
	asm volatile("cli");
	char res = buf->buf[0];
	buf->size = 0;
	
	return res;
}
void getbackStr(InputBuf *buf, char *dst)
{
	asm volatile("sti");
	while (!buf->size || buf->buf[buf->size - 1] != '\n')
		waitForInterrupt();
	asm volatile("cli");
	int i = 0;
	for (; i < buf->size && buf->buf[i] != '\n'; ++i)
		dst[i] = buf->buf[i];
	dst[i] = 0;
	/*
	for (int j = 0; i < buf->size; ++i, ++j)
		buf->buf[j] = buf->buf[i];
	buf->size = i;
	*/
	buf->size = 0;
}


void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%es"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%fs"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%gs"::"a"(KSEL(SEG_KDATA)));
	switch(tf->irq) {
		// TODO: 填好中断处理程序的调用
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf);
			break;
		case 0x20:
			timerHandle(tf);
			break;
		case 0x21:
			KeyboardHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
			
		default:
		log("unknown irq\n");
		assert(0);
	}
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}

void KeyboardHandle(struct TrapFrame *tf){
	uint32_t code = getKeyCode();
	if(code == 0xe){ // 退格符
		// TODO: 要求只能退格用户键盘输入的字符串，且最多退到当行行首
		if (displayCol && deleteBack(&inputBuf))
		{
			--displayCol;
			updateCursor(displayRow, displayCol);
			printChar(' ');
		}
	}else if(code == 0x1c){ // 回车符
		// TODO: 处理回车情况
		if (displayRow == 24)
			scrollScreen();
		else
			++displayRow;
		displayCol = 0;
		insertBuf(&inputBuf, '\n');
		putChar('\n');
	}else if (code < 0x81){ // 正常字符
		// TODO: 注意输入的大小写的实现、不可打印字符的处理
		putChar(getChar(code));
		printChar(getChar(code));
		insertBuf(&inputBuf, getChar(code));
		if (displayCol == 79)
		{
			displayCol = 0;
			if (displayRow == 24)
				scrollScreen();
			else
				++displayRow;
		}
		else
			++displayCol;
		
	}
	updateCursor(displayRow, displayCol);
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallRead(tf);
			break; // for SYS_READ
		default:break;
	}
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			syscallPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct TrapFrame *tf) {
	log("syscall print\n");
	int sel =  USEL(SEG_UDATA);//TODO: segment selector for user data, need further modification
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	//int pos = 0;
	char character = 0;
	//uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		// TODO: 完成光标的维护和打印到显存
		if (character == '\n')
		{
			if (displayRow == 24)
				scrollScreen();
			else
				++displayRow;
			displayCol = 0;
		}
		else
		{
			printChar(character);
			if (displayCol == 79)
			{
				displayCol = 0;
				if (displayRow == 24)
					scrollScreen();
				else
					++displayRow;
			}
			else
				++displayCol;
		}
	}
	
	updateCursor(displayRow, displayCol);
}

void syscallRead(struct TrapFrame *tf){
	switch(tf->ecx){ //file descriptor
		case 0:
			syscallGetChar(tf);
			break; // for STD_IN
		case 1:
			syscallGetStr(tf);
			break; // for STD_STR
		default:break;
	}
}
void printChar(char c)
{
	uint16_t data = c | (0x0c << 8);
	int pos = (80 * displayRow + displayCol) * 2;
	asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
}
void syscallGetChar(struct TrapFrame *tf){
	// TODO: 自由实现
	tf->eax = getbackChar(&inputBuf);
}

void syscallGetStr(struct TrapFrame *tf){
	// TODO: 自由实现
	getbackStr(&inputBuf, (char *)tf->edx);
}

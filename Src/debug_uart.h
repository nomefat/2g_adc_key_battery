#ifndef _DEBUG_UART_
#define _DEBUG_UART_



#define DEBUG_DOUBLE_BUFF_LEN 2048

#define Q_LEN 512           //队列长度

extern char debug_send_buff[Q_LEN];

typedef struct _debug_double_buff
{
	unsigned int len;
	char data[DEBUG_DOUBLE_BUFF_LEN];
}struct_debug_double_buff;


void start_from_debug_dma_receive(void);

int copy_string_to_double_buff(const char *pstr);

void uart_from_debug_idle_callback(void);

void task_debug(void);
	
void sys_print(void);
	


#define debug(str) copy_string_to_double_buff(str)


#endif

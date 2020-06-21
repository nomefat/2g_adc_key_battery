#ifndef GPRS_HAL_H_
#define GPRS_HAL_H_


#define LED_R 1
#define LED_G 2
#define LED_B 3

#define LED_ON 1
#define LED_OFF 2


void grps_power_off(void);

void grps_power_on(void);


void led_ctrl(uint8_t led,uint8_t stat);

void led_flash(uint8_t led );


void start_from_gprs_dma_receive(void);

void uart_from_gprs_idle_callback(void);


void gprs_uart_send_string(char *pstr);


void gprs_uart_send_bin(void *pdata,uint16_t len);






#endif


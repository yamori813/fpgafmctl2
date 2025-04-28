/* -----------------------------------------------------------------------
 * Title:    Header 2313 test
 * Author:   Alexander Weber alex@tinkerlog.com
 * Date:     21.12.2008
 * Hardware: ATtiny2313V
 * Software: AVRMacPack
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "globals.h"
#include "uart.h"
#include "usi_i2c_master.h"

#define LCD_ADDR	0x4e

#define LCD_RS		(1 << 0)
#define	LCD_RW		(1 << 1)
#define LCD_E		(1 << 2)
#define LCD_DATA_SHIFT	4

void init_ir();
void chk_ir();

uint8_t bk = 0;

void lcd_putc(char c)
{
unsigned char buf[4];

   buf[0] = LCD_ADDR;
   buf[3] =  buf[1] = bk | 0x01 | (c & 0xf0);
   buf[2] = bk | 0x0d | (c & 0xf0);
   USI_I2C_Master_Start_Transmission(buf, 4);
   buf[0] = LCD_ADDR;
   buf[3] = buf[1] = bk | 0x01 | ((c & 0xf) << 4);
   buf[2] = bk | 0x0d | ((c & 0xf) << LCD_DATA_SHIFT);
   USI_I2C_Master_Start_Transmission(buf, 4);
}

void lcd_ctl(char c)
{
unsigned char buf[4];

  buf[0] = LCD_ADDR;
  buf[3] = buf[1] = bk | (c & 0xf0);
  buf[2] = bk | 0x04 | (c & 0xf0);
  USI_I2C_Master_Start_Transmission(buf, 4);
  buf[0] = LCD_ADDR;
  buf[3] = buf[1] = bk | ((c & 0xf) << 4);
  buf[2] = bk | 0x04 | ((c & 0xf) << LCD_DATA_SHIFT);
  USI_I2C_Master_Start_Transmission(buf, 4);
}

int main(void) {

  uint8_t i = 0;
  uint16_t c = 0;
  unsigned char buf[4];

  DDRD |= (1 << LED_BIT);
   
  for (i = 0; i < 5; i++) {
    PORTD |= (1 << LED_BIT);
    _delay_ms(50);
    PORTD &= ~(1 << LED_BIT);
    _delay_ms(50);
  }

  init_ir();

  init_uart();

  // i2c master init
  DDR_USI  |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
  PORT_USI |= (1 << PORT_USI_SCL);
  PORT_USI |= (1 << PORT_USI_SDA);
  USIDR = 0xFF;
  USICR = (0 << USISIE) | (0 << USIOIE) | (1 << USIWM1) | (0 << USIWM0) |
    (1 << USICS1) | (0 << USICS0) | (1 << USICLK) | (0 << USITC);
  USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF)  | (1 << USIDC)  |
    (0x00 << USICNT0);

  for (i = 0; i < 3; i++) {
    buf[0] = LCD_ADDR;
    buf[3] = buf[1] = 0x30;
    buf[2] = 0x34;
    USI_I2C_Master_Start_Transmission(buf, 4);
    _delay_ms(5);
  }

  buf[0] = LCD_ADDR;
  buf[3] = buf[1] = 0x20;
  buf[2] = 0x24;
  USI_I2C_Master_Start_Transmission(buf, 4);

  /* function setup */
  lcd_ctl(0x28);

  /* display on, cursole on, blink on */
  lcd_ctl(0x0f);

  /* clear display */
  lcd_ctl(0x01);

  /* entory mode setup */
  lcd_ctl(0x06);

  bk = 0x08;

  sei();

  i = 0;
  while (1) {

    c = uart_getc();
    if (c != UART_NO_DATA) {
/*
      ++i;
      if(i > 80) {
         lcd_ctl(0x01);
         lcd_ctl(0x02);
         i = 0;
      }
*/
      if (c < 0x20) {
        ++i;
        if (i % 3)
          c = ' ';
        else
          c = 0;
      }
      if (c)
        lcd_putc(c);
    }
    chk_ir();
    _delay_ms(100);
/*
    PORTD |= (1 << LED_BIT);
    _delay_ms(200);
    PORTD &= ~(1 << LED_BIT);
    _delay_ms(800);
*/
  }

  return 0;

}


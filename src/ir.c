// This code is fpga fm tuner control by ir remote controller
// signal is sony TV and CD remote controller
// This code based on as follow code. But also copy from other location
// https://github.com/itdaniher/Thermocouple-Interfaces
//

#include "globals.h"
#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#define IR_REC	2

#define STARTLOWLEN 72

volatile int ispc;
volatile int insleep;
volatile int irstat;
volatile int locount;
volatile int hicount;
volatile int bitcount;
volatile long lastvalue;
volatile long irvalue;

void selectst(int st, int level, int muti, int stattx, int band, int mono, int mute, int sample);

//uint16_t jcom_tokyo_freq[] EEMEM = {768,774,783,789,803,809,816,822,828};
uint16_t jcom_tokyo_freq[]  = {768,774,783,789,803,809,816,822,828};

#if 0
ISR(INT1_vect)
{
	irstat = 3;
}
#endif

ISR(INT0_vect)
{
	if((PIND >> IR_REC) & 0x01) {
		locount = TCNT0;
	} else {
		hicount = TCNT0;
	}
	if(irstat == 0 && ((PIND >> IR_REC) & 0x01) && locount > STARTLOWLEN) {
		irstat = 1;
		bitcount = 0;
		irvalue = 0;
		//		TCCR0A = 0b00000010;
		//		OCR0A = 50;
		PORTD |= (1 << LED_BIT);
	} else if(irstat == 1) {
		if((PIND >> IR_REC) & 0x01) {
			if(locount > hicount * 2) {
				irvalue = (irvalue << 1) | 1;
			} else {
				irvalue = (irvalue << 1);
			}
			++bitcount;
			if(bitcount == 12) {
				irstat = 2;
				PORTD &= ~(1 << LED_BIT);
			}
		}
	} else if(irstat == 2) {
	}
	TCNT0 = 0x00;
}

char hexchar(int val)
{
	if(val <= 9) {
		return '0' + val;
	} else {
		return 'A' + (val - 10);
	}
}

// https://hirokun.jp/av/fpga-tuner_fc.html

// 0		1		2		3
// 192.0	170.7		153.6		139.6
// 4		5		6		7
// 128.0	118.2		109.7		102.4	
// 8		9		10		11
// 96.0		90.4		85.3		80.8
// 12		13		14		15
// 76.8		73.1		69.8		66.8
// default 3
int level = 3;

// 0      1      2      3
// 236kHz 198kHz 162kHz 126kHz
// default 1
int band = 1;
int stattx = 0; // 0 = off, 1 = on
int muti = 0; // Multi path off = 0, on = 1

int sample = 0; // 0 = 48k, 1 = 96K, 2 = 192K, 3 = 192K 
int mute = 0; // Muting 0 = Off, 1 = on
int mono = 0; // 1 = mono, 0 = Stereo Auto

int idlecount;

int tune = 0;
long lastsend = 0;

void init_ir ( void )
{
	// 外部割り込みマスクレジスタ
	GIMSK |= (1<<INT0);

	TCCR0B |= (1<<CS02);
	irstat = 0;
	sei();

	// INTピンの論理変化で割り込み
	MCUCR |= (1<<ISC00);
	idlecount = 0;
	insleep = 0;
}

void chk_ir()
{
#if 0
		if(irstat == 0) {
			if(idlecount > 10) { // 200ms * 10 = 2 sec
//				PORTB &= ~(1 << BIT_LED); // LED on
				// INTピンのLレベルで割り込みに変更
				MCUCR &= ~(1<<ISC00);
				MCUCR &= ~(1<<ISC01);
				// PINチェンジ割り込み設定
				GIMSK |= (1 << PCIE);
				// Timer0停止
//				TCCR0B = 0;
				insleep = 1;

				sleep_mode();

				insleep = 0;
				idlecount = 0;
//				PORTB |= 1 << BIT_LED; // LED off
				// INTピンの論理変化で割り込みに戻す
				MCUCR |= (1<<ISC00);
//				TCCR0B |= (1<<CS02);
				// PINチェンジ割り込み禁止
				GIMSK &= ~(1 << PCIE);
			} else {
				++idlecount;
			}
		}
	if (irstat == 3) {
		++tune;
		if (tune == 8)
			tune = 0;
		selectst(tune, level, muti, stattx, band, mono, mute, sample);
		irstat = 0;
	}
#endif
	if(irstat == 2) {
		if(lastvalue == irvalue && lastsend != irvalue) {
			lastsend = irvalue;
			GIMSK &= ~(1<<INT0);
			int button = 0;
			switch (irvalue) {
				case 0x010:   // 1
				case 0x011:   // 1
					button = 1;
					break;
				case 0x810:   // 2
				case 0x811:   // 2
					button = 2;
					break;
				case 0x410:   // 3
				case 0x411:   // 3
					button = 3;
					break;
				case 0xc10:   // 4
				case 0xc11:   // 4
					button = 4;
					break;
				case 0x210:   // 5
				case 0x211:   // 5
					button = 5;
					break;
				case 0xa10:   // 6
				case 0xa11:   // 6
					button = 6;
					break;
				case 0x610:   // 7
				case 0x611:   // 7
					button = 7;
					break;
				case 0xe10:   // 8
				case 0xe11:   // 8
					button = 8;
					break;
				case 0x110:   // 9
				case 0x111:   // 9
					button = 9;
					break;
				case 0x910:   // 10
				case 0x510:   // 11
				case 0xd10:   // 12
				default:
					button = 0;
					break;
			}
			if(button != 0) {
				selectst(button -1, level, muti, stattx, band, mono, mute, sample);
			}
			GIMSK |= (1<<INT0);
		}
		lastvalue = irvalue;
		irstat = 0;
	}
}

void selectst(int st, int level, int muti, int stattx, int band, int mono, int mute, int sample)
{
//	int fqint = eeprom_read_word(&jcom_tokyo_freq[st]);
	int fqint = jcom_tokyo_freq[st];

	uart_putc((fqint / 100) + '0');
	uart_putc((fqint / 10) % 10 + '0');
	uart_putc(fqint % 10 + '0');
	uart_putc(hexchar(level));
	uart_putc(hexchar(muti << 3 | stattx << 2 | band));
	uart_putc(hexchar(mono << 3 | mute << 2 | sample));
	uart_putc('\n');
}

#include "libCORE.h"
#include "libUART.h"
#include "libTIM.h"

#include "stdlib.h"

#include "decoder.h"

extern uart_irq com;
extern gpio led_ir;
extern gpio phototransistor;

extern timer_basic timer_us;

uint8_t data[20];
extern volatile bool rawDataEnabled;

void delay_us(uint16_t us)
{
	timer_us.setCounter(0);
	while (timer_us.getCounter() < us);
}

uint8_t receiveByte(void)
{
	uint8_t temp = 0;

	for (uint8_t i = 0; i < 8; i ++)
	{
		led_ir = LOW;
		delay_us(CLOCK_DELAY_US);
		led_ir = HIGH;

		/*if (phototransistor == HIGH) temp |= (1 << i);
		delay_us(CLOCK_DELAY_US);*/

		delay_us(CLOCK_DELAY_US / 2);
		if (phototransistor == HIGH) temp |= (1 << i);
		delay_us(CLOCK_DELAY_US / 2);
	}

	return temp;
}

bool receiveMessage(void)
{
	led_ir = LOW;
	tick::delay(10);
	led_ir = HIGH;

	uint32_t start = tick::get();
	while (phototransistor == HIGH)
	{
		if (tick::get() - start > 500) return false;
	}

	for (uint8_t i = 0; i < 20; i ++) data[i] = receiveByte();

	return 1;
}

void processMessage(void)
{
	if (rawDataEnabled)
	{
		com.print("RAW DATA: ");

		for (uint8_t i = 0; i < 20; i++)
		{
			char buffer[8];
			itoa(data[i], buffer, 16);
			com.print(buffer);
			com.print(" ");
		}

		com.print("\n");
		return;
	}

	//decode the output
	if (data[0] & 0x01)
		com.print("AUTO ");
	if ((data[0] >> 7) & 0x01)
		com.print("AVG ");
	if ((data[0] >> 6) & 0x01)
		com.print("MIN ");
	if ((data[0] >> 5) & 0x01)
		com.print("MAX ");
	if ((data[0] >> 2) & 0x01)
		com.print("CREST ");
	if ((data[0] >> 1) & 0x01)
		com.print("REC ");
	if ((data[0] >> 3) & 0x01)
		com.print("HOLD ");
	if (data[2] & 0x01)
		com.print("DELTA ");
	if (data[9] & 0x01)
		com.print("BEEP ");

	//DECODE MAIN DISPLAY
	com.print("MAIN: ");

	if ((data[1] >> 7) & 0x01)
		com.print("-");

	for (uint8_t i = 0; i < 6; i++)
	{
		char temp[] = {(char)(48 + decodeDigit(data[2 + i])), '\0'};
		com.print(temp);
		if ((data[3 + i] & 0x01) & (i < 4))
			com.print(".");
	}
	com.print(" ");

	//DECODE UNIT PREFIX FOR MAIN DISPLAY
	if ((data[13] >> 6) & 0x01)
		com.print("n");
	if ((data[14] >> 3) & 0x01)
		com.print("u");
	if ((data[14] >> 2) & 0x01)
		com.print("m");
	if ((data[14] >> 6) & 0x01)
		com.print("k");
	if ((data[14] >> 5) & 0x01)
		com.print("M");

	//DECODE UNIT FOR MAIN DISPLAY
	if (data[7] & 0x01)
		com.print("V");
	if ((data[13] >> 7) & 0x01)
		com.print("A");
	if ((data[13] >> 5) & 0x01)
		com.print("F");
	if ((data[13] >> 4) & 0x01)
		com.print("S");
	if ((data[14] >> 7) & 0x01)
		com.print("D%");
	if ((data[14] >> 4) & 0x01)
		com.print("Ohm");
	if ((data[14] >> 1) & 0x01)
		com.print("dB");
	if (data[14] & 0x01)
		com.print("Hz");
	com.print(" ");

	//DC OR AC
	if ((data[0] >> 4) & 0x01)
		com.print("DC ");
	if (data[1] & 0x01)
		com.print("AC ");

	//DECODE AUXILIARY DISPLAY
	com.print("AUX: ");

	if ((data[8] >> 4) & 0x01)
		com.print("-");

	for (uint8_t i = 0; i < 4; i++)
	{
		char temp[] = {(char)(48 + decodeDigit(data[9 + i])), '\0'};
		com.print(temp);
		if ((data[10 + i] & 0x01) & (i < 3))
			com.print(".");
	}

	com.print(" ");

	//DECODE UNIT PREFIX FOR AUXILIARY DISPLAY
	if ((data[8] >> 1) & 0x01)
		com.print("m");
	if (data[8] & 0x01)
		com.print("u");
	if ((data[13] >> 1) & 0x01)
		com.print("k");
	if (data[13] & 0x01)
		com.print("M");

	//DECODE UNIT FOR AUXILIARY DISPLAY
	if ((data[13] >> 2) & 0x01)
		com.print("Hz");
	if ((data[13] >> 3) & 0x01)
		com.print("V");
	if ((data[8] >> 2) & 0x01)
		com.print("A");
	if ((data[8] >> 3) & 0x01)
		com.print("%4-20mA");

	com.print(" ");
	if ((data[13] >> 2) & 0x01)
		com.print("AC");

	//FINISH
	com.print("\n");

	//detect low battery
	if ((data[8] >> 7) & 0x01)
		com.print(" LOW BAT\n");
}

int8_t decodeDigit(uint8_t source)
{
	int8_t result = 0;

	switch (source >> 1)
	{
		case 0b1011111: result = 0; break;
		case 0b1010000: result = 1; break;
		case 0b1101101: result = 2; break;
		case 0b1111100: result = 3; break;
		case 0b1110010: result = 4; break;
		case 0b0111110: result = 5; break;
		case 0b0111111: result = 6; break;
		case 0b1010100: result = 7; break;
		case 0b1111111: result = 8; break;
		case 0b1111110: result = 9; break;
		case 0b1111001: result = ('d' - 48); break;
		case 0b0010000: result = ('i' - 48); break;
		case 0b0111001: result = ('o' - 48); break;
		case 0b0001011: result = ('L' - 48); break;
		case 0b0000000: result = (' ' - 48); break;
		default: result = ('?' - 48); break;
	}

	return result;
}

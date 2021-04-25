#include "libCORE.h"
#include "libGPIO.h"
#include "libUART.h"
#include "libNVM.h"
#include "libTIM.h"

#include "version.h"
#include "decoder.h"

gpio led_red{GPIOA, 7};
gpio led_ir{GPIOA, 1};
gpio phototransistor{GPIOA, 0};

uart_irq com{USART2};

timer_basic timer_us{TIM2};			//timer for microsend delay generation

volatile mode_t mode;
volatile bool rawDataEnabled;

char com_txBuffer[128];
char com_rxBuffer[4];

void comIdleLineCB(void);

int main(void)
{
	rcc::initHSI();
	rcc::initSystemClock(rcc::vRange::high, rcc::sysclk::HSI);
	rcc::disableMSI();

	led_red.initOutput(gpio::speed::low, gpio::output::high, gpio::type::opendrain);
	led_ir.initOutput(gpio::speed::low, gpio::output::high, gpio::type::opendrain);
	phototransistor.initInput();

	com.init(115200);
	com.enableTX(gpio{GPIOA, 9, gpio::af::_4}, com_txBuffer, sizeof(com_txBuffer));
	com.enableRX(gpio{GPIOA, 10, gpio::af::_4}, com_rxBuffer, sizeof(com_rxBuffer));
	com.enableIdleLine(comIdleLineCB);
	NVIC_EnableIRQ(USART2_IRQn);

	timer_us.init(16, 0xFFFF);
	timer_us.start();

	libASSERT(false);

	//print initial info on the serial port
	com.print("\n\nBrymen 867/869 interface cable\nfor more info, see embedblog.eu/?p=475\n\n");		//TODO:web
	com.printf("Firmware revision: %s\n", _V_BUILD_TAG);
	com.print("Available commands:\n");
	com.print("F - Five samples per second\n");
	com.print("O - One sample per second\n");
	com.print("S - Stop autosend\n");
	com.print("D - read single Data\n");
	com.print("R - toggle Raw data output\n");
	com.print("E - save current settings to Eeprom\n");

	eeprom::enableClock();
	uint8_t data[2];
	eeprom::read(0, data, sizeof(data));
	if (data[0] == EEPROM_VALID_FLAG)
	{
		mode = (mode_t)data[1];
		com.print("Valid settings loaded from eeprom\n");
	}

	while (1)
	{
		if (receiveMessage())
		{
			led_red = LOW;
			processMessage();
			tick::delay(50);
			led_red = HIGH;

			if (mode == mode_t::sendRate1Hz) tick::delay(920);
			else if (mode == mode_t::sendRate5Hz) continue;
			else
			{
				mode = mode_t::stop;
				while (mode == mode_t::stop);
			}
		}
	}
}

void comIdleLineCB(void)
{
	switch(com_rxBuffer[0])
	{
		case 'F': mode = mode_t::sendRate5Hz; break;
		case 'O': mode = mode_t::sendRate1Hz; break;
		case 'S': mode = mode_t::stop; break;
		case 'D': mode = mode_t::sendSingleSample; break;
		case 'R': rawDataEnabled = !rawDataEnabled; break;
		case 'E':
			uint8_t data[2] = {EEPROM_VALID_FLAG, (uint8_t)mode};
			eeprom::write(0, data, sizeof(data));
			break;
	}

	com.rxReset();
}

extern "C" void USART2_IRQHandler(void)
{
	com.IRQHandler();
}

void custom_assert(const char* file, uint32_t line)
{
	com.printf("ERROR! Assert failed: file %s, line %d", file, line);
	while (com.txCharsRemaining());
	#ifdef DEBUG
	__asm("bkpt 0");
	#endif
}

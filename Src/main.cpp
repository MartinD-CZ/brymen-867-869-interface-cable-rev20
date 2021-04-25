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

void comReceivedCB(void);

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
	com.enableRxReceivedNum(1, comReceivedCB);
	NVIC_EnableIRQ(USART2_IRQn);

	timer_us.init(16, 0xFFFF);
	timer_us.start();

	//print initial info on the serial port
	com.print("\n\nBrymen 867/869 interface cable\nfor more info, see embedblog.eu/?p=475\n\n");		//TODO:web
	com.printf("Firmware revision: %s\n", _V_BUILD_TAG);
	com.print("Available commands:\n");
	com.print("F - 5 samples per second\n");
	com.print("O - 1 sample per second\n");
	com.print("S - stop autosend\n");
	com.print("D - send a single reading\n");
	com.print("R - toggle raw data output\n");

	/*eeprom::enableClock();
	uint64_t test = 0xDEADBEEFABCDEFC4;
	eeprom::write<uint64_t>(0, test);

	uint64_t readback;
	eeprom::read<uint64_t>(0, &readback);

	com.printf("Data readback: 0x%x\n", readback);*/

	while (1)
	{
		if (receiveMessage())
		{
			led_red = LOW;
			processMessage();
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

void comReceivedCB(void)
{
	switch(com_rxBuffer[0])
	{
		case 'F': mode = mode_t::sendRate5Hz; break;
		case 'O': mode = mode_t::sendRate1Hz; break;
		case 'S': mode = mode_t::stop; break;
		case 'D': mode = mode_t::sendSingleSample; break;
		case 'R': rawDataEnabled = !rawDataEnabled; break;
		default: libASSERT(false);
	}

	com.rxReset();
}

extern "C" void USART2_IRQHandler(void)
{
	com.IRQHandler();
}

void __custom__assert(const char* file, uint32_t line)
{
	com.printf("ERROR! Assert failed: file %s, line %d", file, line);
	while (com.txCharsRemaining());
	#ifdef DEBUG
	__asm("bkpt 0");
	#endif
}

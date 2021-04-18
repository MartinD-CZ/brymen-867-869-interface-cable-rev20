#include "libCORE.h"
#include "libGPIO.h"
#include "libUART.h"
#include "libNVM.h"

#include "version.h"

gpio led_red{GPIOA, 7};
gpio led_ir{GPIOA, 1};
gpio phototransistor{GPIOA, 0};

uart_irq com{USART2};

char com_txBuffer[128];
char com_rxBuffer[4];

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
	NVIC_EnableIRQ(USART2_IRQn);
	//com.enableTX(gpio{GPIOA, 9, gpio::af::_4}, DMA1_Channel4, dmaPriority::medium, com_buffer, sizeof(com_buffer));

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
		tick::delay(500);
		led_red.toggle();
		com.print("tick\n");
	}
}

extern "C" void USART2_IRQHandler(void)
{
	com.IRQHandler();
}

void __custom__assert(const char* file, uint32_t line)
{
	com.printf("ERROR! Assert failed: file %s, line %d", file, line);
	__asm("bkpt 0");
}

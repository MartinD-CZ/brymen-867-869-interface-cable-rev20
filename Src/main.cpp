#include "libCORE.h"
#include "libGPIO.h"
#include "libUART.h"

gpio rled{GPIOA, 7};

int main(void)
{
	rcc::initHSI();
	rcc::initSystemClock(rcc::vRange::high, rcc::sysclk::HSI);
	rcc::disableMSI();

	rled.initOutput(gpio::speed::low, gpio::output::high, gpio::type::opendrain);

	while (1)
	{
		tick::delay(500);
		rled.toggle();
	}
}

#ifndef DECODER_H_
#define DECODER_H_

enum class mode_t
{
	sendRate1Hz,
	sendRate5Hz,
	stop,
	sendSingleSample
};

bool receiveMessage(void);
void processMessage(void);
int8_t decodeDigit(uint8_t source);

#endif /* DECODER_H_ */

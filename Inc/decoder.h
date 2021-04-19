#ifndef DECODER_H_
#define DECODER_H_

uint8_t receiveByte(void);
bool receiveMessage(void);
void processData(void);
int8_t decodeDigit(uint8_t source);

#endif /* DECODER_H_ */

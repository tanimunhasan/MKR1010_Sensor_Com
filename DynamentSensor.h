/*
 * DynamentSensor.h
 *
 *  Created on: 15 Apr 2026
 *      Author: Sayed
 */

#ifndef DYNAMENTSENSOR_H_
#define DYNAMENTSENSOR_H_

#include<Arduino.h>

class DynamentSensor
{
public:
	enum ResponseType
	{
		NO_RESPONSE = 0,
		NEW_DATA,
		NEW_DATA_OUTLIER,
		INVALID_DATA,
		FRAME_CRC_ERROR,
		TIMEOUT_ERROR,
		SEND_ERROR
	};

	explicit DynamentSensor(HardwareSerial& serialPort);

	void begin(uint32_t baud = 9600);

	bool sendLiveData2Request();
	bool readGasValueBlocking(float& gasValue, uint32_t timeoutMs = 500, bool flushInputBeforeSend=true);
	bool poll();
	bool availableMessage() const;
	ResponseType getResponse() const;
	float getLatestGasValue() const;
	float getLatestGasValue2() const;
	uint16_t getLatestStatus1() const;
	uint16_t getLatestStatus2() const;

private:
	// Protocol constants
    static const uint8_t READ_VAR         = 0x13;
    static const uint8_t DLE              = 0x10;
    static const uint8_t WRITE_REQUEST    = 0x15;
    static const uint8_t ACK              = 0x16;
    static const uint8_t NAK              = 0x19;
    static const uint8_t DAT              = 0x1A;
    static const uint8_t EOF_MARK         = 0x1F;

    static const uint8_t WRITE_PASSWORD_1 = 0xE5;
    static const uint8_t WRITE_PASSWORD_2 = 0xA2;

    static const uint8_t LIVE_DATA_SIMPLE = 0x06;
    static const uint8_t LIVE_DATA_2      = 0x2C;

    static const uint16_t DYNAMENT_MAX_PACKET_SIZE = 300;
    static const uint16_t DYNAMENT_MAX_VALID       = 2000;

    static const uint8_t CSUM_STANDARD = 0;
    static const uint8_t CSUM_CRC      = 1;
    static const uint8_t CSUM_TYPE     = CSUM_STANDARD;

    static const uint16_t CRC_POLYNOMIAL = 0x8005;
    static const uint16_t CRC_VALUE_MASK = 0x8000;

    HardwareSerial& _serial;

    //RX state
    bool _receivingPacket;
    uint8_t _rxBuffer[DYNAMENT_MAX_PACKET_SIZE];
    uint16_t _rxCount;
    bool _dleReceived;
    uint8_t _command;
    bool _eofReceived;
    uint16_t _rcvCsum;
    uint16_t _calcCsum;
    bool _packetComplete;
    bool _csumError;
    int _csumByteReceived;
    bool _packetNAKed;
    uint8_t _errorCode;
    bool _packetACKed;
    bool _latestPacketValid;

    // Message/result state
    ResponseType _msgResponse;
    bool _msgReceived;

    // Parsed live values
    float _gasValue1;
    float _gasValue2;
    uint16_t _latestStatus1;
    uint16_t _latestStatus2;

    uint16_t updateChecksum(uint16_t currentCRC, uint8_t newByte);
    uint16_t updateCRCTab(uint16_t index);

    void resetState();
    void packetSent();

    bool sendPacket(uint8_t cmd, uint8_t variableID, uint8_t dlen, const uint8_t* dataPtr);
    bool processIncomingByte(uint8_t chr);
    void processReceivedPacket();
    ResponseType readLiveData2Response(const uint8_t* dataPtr, int len);

    static float uint32ToFloat(uint32_t raw);

};



#endif /* DYNAMENTSENSOR_H_ */

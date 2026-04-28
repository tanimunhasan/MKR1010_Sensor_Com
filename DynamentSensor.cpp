/*
 * DynamentSensor.cpp
 *
 *  Created on: 15 Apr 2026
 *      Author: Sayed
 */

#include "DynamentSensor.h"
#include <string.h>
#include "app.h"


DynamentSensor::DynamentSensor(HardwareSerial& serialPort)
						:_serial(serialPort),
						 _receivingPacket(false),
						 _rxCount(0),
						 _dleReceived(false),
						 _command(0),
						 _eofReceived(false),
						 _rcvCsum(0),
						 _calcCsum(0),
						 _packetComplete(false),
						 _csumError(false),
						 _csumByteReceived(0),
						 _packetNAKed(false),
						 _errorCode(0),
						 _packetACKed(false),
						 _latestPacketValid(false),
						 _msgResponse(NO_RESPONSE),
						 _msgReceived(false),
						 _gasValue1(0.0f),
						 _gasValue2(0.0f),
						 _latestStatus1(0),
						 _latestStatus2(0) {
}

void DynamentSensor::begin(uint32_t baud)
{
	_serial.begin(baud);
	resetState();
	_msgResponse = NO_RESPONSE;
	_msgReceived = false;
}

bool DynamentSensor::availableMessage()const
{
	return _msgReceived;
}

DynamentSensor::ResponseType DynamentSensor::getResponse() const
{
	return _msgResponse;
}

float DynamentSensor::getLatestGasValue() const
{
	return _gasValue1;
}

float DynamentSensor::getLatestGasValue2() const
{
    return _gasValue2;
}

uint16_t DynamentSensor::getLatestStatus1() const
{
	return  _latestStatus1;
}

uint16_t DynamentSensor::getLatestStatus2() const
{
	return _latestStatus2;
}

uint16_t DynamentSensor::updateChecksum(uint16_t currentCRC, uint8_t newByte)
{
    if (CSUM_TYPE == CSUM_CRC)
    {
        return static_cast<uint16_t>(
            (currentCRC << 8) ^ updateCRCTab(static_cast<uint16_t>((currentCRC >> 8) ^ newByte))
        );
    }
    else
    {
        return static_cast<uint16_t>(currentCRC + static_cast<uint16_t>(newByte));
    }
}

uint16_t DynamentSensor::updateCRCTab(uint16_t index)
{
    uint16_t uiCrcValue = 0;
    uint16_t uiTempCrcValue = static_cast<uint16_t>(index << 8);

    for (uint16_t i = 0; i < 8; i++)
    {
        if (((uiCrcValue ^ uiTempCrcValue) & CRC_VALUE_MASK) > 0)
        {
            uiCrcValue = static_cast<uint16_t>((uiCrcValue << 1) ^ CRC_POLYNOMIAL);
        }
        else
        {
            uiCrcValue = static_cast<uint16_t>(uiCrcValue << 1);
        }
        uiTempCrcValue = static_cast<uint16_t>(uiTempCrcValue << 1);
    }

    return uiCrcValue;
}

void DynamentSensor::resetState()
{
	_receivingPacket = false;
	_rxCount = 0;
	_dleReceived = false;
	_command= 0;
	_eofReceived = false;
	_rcvCsum = 0;
	_packetComplete = false;
	_csumError = false;
	_csumByteReceived = 0;
	_packetNAKed = false;
	_packetACKed = false;
	_errorCode = 0;
	_latestPacketValid = false;
}

void DynamentSensor::packetSent()
{
	resetState();
}

bool DynamentSensor::sendLiveData2Request()
{
	_msgReceived = false;
	_msgResponse = NO_RESPONSE;
	return sendPacket(READ_VAR, LIVE_DATA_2, 0, nullptr);
}

bool DynamentSensor::sendPacket(uint8_t cmd, uint8_t variableID, uint8_t dlen, const uint8_t* dataPtr)
{
    uint8_t txBuf[DYNAMENT_MAX_PACKET_SIZE];
    uint16_t txBufPtr = 0;
    uint16_t csum = 0;

    txBuf[txBufPtr++] = DLE;
    csum = updateChecksum(csum, DLE);

    txBuf[txBufPtr++] = cmd;
    csum = updateChecksum(csum, cmd);

    if (cmd == READ_VAR)
    {
        txBuf[txBufPtr++] = variableID;
        csum = updateChecksum(csum, variableID);
    }
    else if (cmd == WRITE_REQUEST)
    {
        txBuf[txBufPtr++] = WRITE_PASSWORD_1;
        csum = updateChecksum(csum, WRITE_PASSWORD_1);

        txBuf[txBufPtr++] = WRITE_PASSWORD_2;
        csum = updateChecksum(csum, WRITE_PASSWORD_2);

        txBuf[txBufPtr++] = variableID;
        csum = updateChecksum(csum, variableID);
    }

    if (dlen > 0 && dataPtr != nullptr)
    {
        if (dlen == DLE) {
            txBuf[txBufPtr++] = DLE;
            csum = updateChecksum(csum, DLE);
        }

        txBuf[txBufPtr++] = dlen;
        csum = updateChecksum(csum, dlen);

        for (uint16_t x = 0; x < dlen; x++)
        {
            if (dataPtr[x] == DLE) {
                txBuf[txBufPtr++] = DLE;
                csum = updateChecksum(csum, DLE);
            }

            txBuf[txBufPtr++] = dataPtr[x];
            csum = updateChecksum(csum, dataPtr[x]);

            if (txBufPtr >= DYNAMENT_MAX_PACKET_SIZE - 3)
            {
                return false;
            }
        }
    }

    txBuf[txBufPtr++] = DLE;
    csum = updateChecksum(csum, DLE);

    txBuf[txBufPtr++] = EOF_MARK;
    csum = updateChecksum(csum, EOF_MARK);

    txBuf[txBufPtr++] = static_cast<uint8_t>((csum >> 8) & 0xFF);
    txBuf[txBufPtr++] = static_cast<uint8_t>(csum & 0xFF);

    size_t written = _serial.write(txBuf, txBufPtr);
    _serial.flush();

    if (written != txBufPtr)
    {
        _msgResponse = SEND_ERROR;
        return false;
    }

    packetSent();
    return true;
}


bool DynamentSensor::processIncomingByte(uint8_t chr)
{
    if (_rxCount >= DYNAMENT_MAX_PACKET_SIZE)
    {
        resetState();
    }

    if (chr == DLE && !_eofReceived)
    {
        if (!_receivingPacket)
        {
            _receivingPacket = true;
            _rxCount = 0;
            _rxBuffer[_rxCount++] = chr;
            _calcCsum = updateChecksum(_calcCsum, chr);
            _dleReceived = true;
        }
        else if (_dleReceived)
        {
            _rxBuffer[_rxCount++] = chr;
            _calcCsum = updateChecksum(_calcCsum, chr);
        }
        else
        {
            _dleReceived = true;
            _rxBuffer[_rxCount++] = chr;
            _calcCsum = updateChecksum(_calcCsum, chr);
        }
    }
    else if (chr == EOF_MARK && _dleReceived && !_eofReceived)
    {
        _rxBuffer[_rxCount++] = chr;
        _calcCsum = updateChecksum(_calcCsum, chr);
        _eofReceived = true;
        _csumByteReceived = 0;
        _rcvCsum = 0;
    }
    else if (_eofReceived)
    {
        _rxBuffer[_rxCount++] = chr;
        ++_csumByteReceived;

        if (_csumByteReceived >= 2)
        {
            _rcvCsum = static_cast<uint16_t>(
                (_rxBuffer[_rxCount - 2] << 8) | _rxBuffer[_rxCount - 1]
            );
            _packetComplete = true;

            if (_rcvCsum != _calcCsum)
            {
                _csumError = true;
                _msgResponse = FRAME_CRC_ERROR;
            }
        }
    }
    else if (_packetNAKed)
    {
        _errorCode = chr;
        _packetComplete = true;
        _rxBuffer[_rxCount++] = chr;
    }
    else if (_receivingPacket)
    {
        if (_dleReceived)
        {
            _command = chr;
            if (_command == NAK)
            {
                _packetNAKed = true;
            }
            if (_command == ACK)
            {
                _packetACKed = true;
                _packetComplete = true;
            }
        }

        _rxBuffer[_rxCount++] = chr;
        _calcCsum = updateChecksum(_calcCsum, chr);
    }

    if (chr != DLE)
    {
        _dleReceived = false;
    }

    if (_packetComplete)
    {
        processReceivedPacket();
        _latestPacketValid = true;
        _msgReceived = true;
        return true;
    }

    return false;
}


void DynamentSensor::processReceivedPacket()
{
	if(_rxCount < 3)
	{
		return;
	}

	uint8_t cmd = _rxBuffer[1];
	uint8_t len = _rxBuffer[2];

	if(cmd == DAT) {
		uint8_t rcvData[200] = {0};
		for(uint16_t i = 0; i<len && (i+3)< _rxCount; i++)
		{
			rcvData[i] = _rxBuffer[i+3];
		}
		_msgResponse = readLiveData2Response(rcvData, len);
	}

}

float DynamentSensor::uint32ToFloat(uint32_t raw)
{

	float value = 0.0f;
	memcpy(&value, &raw, sizeof(value));
	return value;
}

DynamentSensor::ResponseType DynamentSensor::readLiveData2Response(const uint8_t* dataPtr, int len)
{
    if (len < 42 || dataPtr == nullptr)
    {
        return INVALID_DATA;
    }

    _latestStatus1 = static_cast<uint16_t>((dataPtr[3] << 8) | dataPtr[2]);
    _latestStatus2 = static_cast<uint16_t>((dataPtr[41] << 8) | dataPtr[40]);

    uint32_t intVal1 =
        (static_cast<uint32_t>(dataPtr[7]) << 24) |
        (static_cast<uint32_t>(dataPtr[6]) << 16) |
        (static_cast<uint32_t>(dataPtr[5]) << 8)  |
        (static_cast<uint32_t>(dataPtr[4]));

    uint32_t intVal2 =
        (static_cast<uint32_t>(dataPtr[15]) << 24) |
        (static_cast<uint32_t>(dataPtr[14]) << 16) |
        (static_cast<uint32_t>(dataPtr[13]) << 8)  |
        (static_cast<uint32_t>(dataPtr[12]));

    _gasValue1 = uint32ToFloat(intVal1);
    _gasValue2 = uint32ToFloat(intVal2);

    if (_latestStatus1 == 0 && _latestStatus2 == 0)
    {
        if (_gasValue1 <= DYNAMENT_MAX_VALID)
        {
            return NEW_DATA;
        }
        else
        {
            return NEW_DATA_OUTLIER;
        }
    }

    return INVALID_DATA;
}

bool DynamentSensor::poll()
{
    while (_serial.available())
    {
        uint8_t ch = static_cast<uint8_t>(_serial.read());
        if (processIncomingByte(ch))
        {
            return true;
        }
    }
    return false;
}

bool DynamentSensor::readGasValueBlocking(float& gasValue,
                                          uint32_t timeoutMs,
                                          bool flushInputBeforeSend)
{
    if (flushInputBeforeSend)
    {
        while (_serial.available())
        {
            _serial.read();
        }
    }

    resetState();
    _msgReceived = false;
    _msgResponse = NO_RESPONSE;

    if (!sendLiveData2Request())
    {
        return false;
    }

    uint32_t startMs = millis();

    while ((millis() - startMs) < timeoutMs)
    {
        while (_serial.available())
        {
            uint8_t ch = static_cast<uint8_t>(_serial.read());
            if (processIncomingByte(ch))
            {
                if (_msgResponse == NEW_DATA || _msgResponse == NEW_DATA_OUTLIER)
                {
                    gasValue = _gasValue1;
                    return true;
                }
                return false;
            }
        }
    }

    _msgResponse = TIMEOUT_ERROR;
    return false;
}

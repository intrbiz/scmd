/**
 * Serial Command Protocol - Chris Ellis
 */

#ifndef SCMD_h
#define SCMD_h

#include <Arduino.h>

/**
 * Serial command protocol
 * 
 * Header
 *  0 - Magic 1
 *  1 - Magic 2
 *  3 - Version
 *  4 - Command
 *  5 - Length MSB
 *  6 - Length LSB
 *  7 - CRC
 *  
 *  Data is sent in fixed length null padded chunks 
 *  of 32 bytes, with a 1 byte CRC
 *  
 */

#define SCMD_MAGIC_1 0x42
#define SCMD_MAGIC_2 0x24
#define SCMD_VERSION 0x02
#define SCMD_ACK     0x55

#define MAX_COMMAND_PAYLOAD_LENGTH 320

#define SCMD_MASTER_ID    0x00

class SCMD
{
    public:
        enum SCMD_STATUS {
            STATUS_OK       = 0x00,
            STATUS_BADLEN   = 0x01,
            STATUS_BADMAGIC = 0x02,
            STATUS_BADVER   = 0x03,
            STATUS_BADCRC   = 0x04,
            STATUS_BADBUF   = 0x05,
            STATUS_BADACK   = 0x06,
            STATUS_NOTUS    = 0x07
        };
        SCMD(Stream& stream) : _stream(stream) { _debug = false; callback = NULL;}
        void initMaster();
        void initDevice(byte deviceId);
        SCMD_STATUS writeAck();
        SCMD_STATUS writeCommand(byte command, byte* payload, uint16_t len);
        SCMD_STATUS writeCommand(byte command, byte target, byte* payload, uint16_t len);
        SCMD_STATUS writeHeader(byte command, byte target, uint16_t len);
        SCMD_STATUS writeDataChunk(byte *data, byte len);
        SCMD_STATUS readAck();
        SCMD_STATUS readCommand(byte *command, byte *from, uint16_t *payloadLength, byte *payload, uint16_t maxPayloadLen);
        SCMD_STATUS readHeader(byte *header);
        SCMD_STATUS readDataChunk(byte *data);
        void setOnReceive(void (*callback)(byte, byte, byte*, uint16_t));
        void debugTo(Stream *stream) { _debugTo = stream; _debug = true; }
        void loop();
        byte computeCRC(const byte *data, byte len);
    private:
        Stream& _stream;
        Stream *_debugTo;
        bool _debug;
        void (*callback)(byte, byte, byte*, uint16_t);
        byte buffer[MAX_COMMAND_PAYLOAD_LENGTH];
        byte _deviceId;
};

#endif

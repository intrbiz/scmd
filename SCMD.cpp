/**
 * SCMD - Serial Command Protocol - Chris Ellis
 */

#include <SCMD.h>

SCMD::SCMD_STATUS SCMD::writeCommand(byte command, byte* payload, uint16_t len)
{
  SCMD_STATUS hs = writeHeader(command, len);
  if (hs != STATUS_OK) return hs;
  for (uint16_t i = 0; i < len; i += 32)
  {
    SCMD_STATUS cs = writeDataChunk(payload + i, (len - i) < 32 ? (len - i) : 32);
    if (cs != STATUS_OK) return cs;
  }
  return STATUS_OK;
}

SCMD::SCMD_STATUS SCMD::writeHeader(byte command, uint16_t len)
{
  // build the 8 byte header
  byte header[8];
  header[0] = SCMD_MAGIC_1;
  header[1] = SCMD_MAGIC_2;
  header[2] = SCMD_VERSION;
  header[3] = command;
  header[4] = 0;
  header[5] = (byte) ((len >> 8) & 0xFF);
  header[6] = (byte) (len & 0xFF);
  header[7] = 0;
  header[7] = computeCRC(header, 8);
  // write the header
  _stream.write(header, 8);
  _stream.flush();
  // wait for an ack
  return readAck();
}

SCMD::SCMD_STATUS SCMD::writeDataChunk(byte *data, byte len)
{
  // build the chunk
  byte chunk[33];
  memset(chunk, 0, sizeof(chunk));
  memcpy(chunk, data, len < (sizeof(chunk) - 1) ? len : (sizeof(chunk) - 1));
  chunk[sizeof(chunk) - 1] = computeCRC(chunk, sizeof(chunk) - 1);
  // write
  _stream.write(chunk, sizeof(chunk));
  _stream.flush();
  // wait for an ack
  return readAck();
}

SCMD::SCMD_STATUS SCMD::writeAck()
{
    _stream.write(SCMD_ACK);
    _stream.flush();
    return STATUS_OK;
}

SCMD::SCMD_STATUS SCMD::readAck()
{
  for (byte i = 0; i < 100; i++)
  {
      if (_stream.available() > 0)
      {
        byte ack = _stream.read();
        return ack == SCMD_ACK ? STATUS_OK : STATUS_BADACK;
      }
      delay(1);
  }
  return STATUS_BADACK;
}

SCMD::SCMD_STATUS SCMD::readCommand(byte *command, uint16_t *payloadLength, byte *payload, uint16_t maxPayloadLen)
{
  // read the header
  byte header[8];
  SCMD_STATUS stat = readHeader(header);
  if (stat != STATUS_OK) return stat;
  // the command
  command[0] = header[3];
  // the length
  payloadLength[0] = (((uint16_t) header[5]) << 8) | ((uint16_t) header[6]);
  if (maxPayloadLen < payloadLength[0]) return STATUS_BADBUF;
  // read in the chunks
  uint16_t read = 0;
  while (read < payloadLength[0])
  {
      SCMD_STATUS cs = readDataChunk(payload);
      if (cs != STATUS_OK) return cs;
      payload += 32;
      read += 32;
  }
  return STATUS_OK;
}

SCMD::SCMD_STATUS SCMD::readHeader(byte *header)
{
  byte len = _stream.readBytes(header, 8);
  if (len != 8) return STATUS_BADLEN;
  if (header[0] != SCMD_MAGIC_1) return STATUS_BADMAGIC;
  if (header[1] != SCMD_MAGIC_2) return STATUS_BADMAGIC;
  if (header[2] != SCMD_VERSION) return STATUS_BADVER;
  // crc check
  byte gotCRC = header[7];
  header[7] = 0;
  byte theCRC = computeCRC(header, 8);
  // is the crc valid ?
  if (gotCRC == theCRC)
  {
    // ack
    writeAck();
    return STATUS_OK;
  }
  else
  {
      return STATUS_BADCRC;
  }
}

SCMD::SCMD_STATUS SCMD::readDataChunk(byte *data)
{
  byte chunk[33];
  byte len = _stream.readBytes(chunk, sizeof(chunk));
  if (len != sizeof(chunk)) return STATUS_BADLEN;
  byte gotCRC = chunk[sizeof(chunk) - 1];
  chunk[sizeof(chunk) - 1] = 0;
  byte theCRC = computeCRC(chunk, sizeof(chunk) - 1);
  if (gotCRC == theCRC)
  {
      // ack
      writeAck();
      // copy into the fucking buffer
      memcpy(data, chunk, 32);
      // all ok
      return STATUS_OK;
  }
  else
  {
    return STATUS_BADCRC;
  }
}

byte SCMD::computeCRC(const byte *data, byte len)
{
  byte crc = 0x00;
  while (len--)
  {
    byte val = *data++;
    for (byte i = 8; i; i--)
    {
      byte sum = (crc ^ val) & 0x01;
      crc >>= 1;
      if (sum) crc ^= 0x8C;
      val >>= 1;
    }
  }
  return crc;
}

void SCMD::setOnReceive(void (*callback)(byte, byte*, uint16_t))
{
    this->callback = callback;
}

void SCMD::loop()
{
    if (_stream.available())
    {
        // read the command
        byte cmd;
        uint16_t len;
        SCMD_STATUS stat = readCommand(&cmd, &len, buffer, sizeof(buffer));
        if (stat == STATUS_OK && callback != NULL)
        {
            // invoke the callback
            callback(cmd, buffer, len);
        }
    }        
}

# SmolTLV - Simple serialization format for JSON/CBOR-like data model for 
# embedded devices
#
# Copyright (c) 2025 Aleš Hakl
# SPDX-License-Identifier: MIT
#

import struct
from io import BytesIO

class DecoderError(Exception):
    pass

SMOLTLV_TYPE_NULL       = 0x00
SMOLTLV_TYPE_BOOL_TRUE  = 0x01
SMOLTLV_TYPE_BOOL_FALSE = 0x02
SMOLTLV_TYPE_INT        = 0x03
SMOLTLV_TYPE_BYTES      = 0x04
SMOLTLV_TYPE_STRING     = 0x05
SMOLTLV_TYPE_LIST       = 0x06
SMOLTLV_TYPE_DICT       = 0x07
SMOLTLV_TYPE_MAX        = 0x08 

class UnknownTLV:
    def __init__(self, type_id, data):
        self.type_id = type_id
        self.data = data
    def __eq__(self, other):
        return isinstance(other, UnknownTLV) \
            and self.type_id == other.type_id \
            and self.data == other.data

class Decoder:
    def __init__(self, fp, allow_unknown_types=False):
        self.fp = fp
        self.position = 0
        self.allow_unknown_types = allow_unknown_types

    def _read_bytes(self, n):
        data = self.fp.read(n)
        if len(data) < n:
            raise DecoderError("Unexpected end of data")
        self.position += n
        return data

    def _read_header(self):
        header = struct.unpack('>I', self._read_bytes(4))[0]
        type_id = (header >> 24) & 0xFF
        length = header & 0xFFFFFF
        return type_id, length

    def decode(self):
        type_id, length = self._read_header()

        if type_id == SMOLTLV_TYPE_DICT:
            items = {}
            end_position = self.position + length
            while self.position < end_position:
                key = self.decode()
                value = self.decode()
                items[key] = value
            return items
        
        if type_id == SMOLTLV_TYPE_LIST:
            items = []
            end_position = self.position + length
            while self.position < end_position:
                items.append(self.decode())
            return items

        data = self._read_bytes(length)

        if type_id == SMOLTLV_TYPE_NULL:  # Null
            if length != 0:
                raise DecoderError("Invalid length for null")
            return None
        
        if type_id == SMOLTLV_TYPE_BOOL_TRUE:  # Boolean True
            if length != 0:
                raise DecoderError("Invalid length for boolean true")
            return True

        if type_id == SMOLTLV_TYPE_BOOL_FALSE:  # Boolean False
            if length != 0:
                raise DecoderError("Invalid length for boolean false")
            return False

        if type_id == SMOLTLV_TYPE_INT:  # Integer
            if length != 8:
                raise DecoderError("Invalid length for integer")
            return struct.unpack('>q', data)[0]

        if type_id == SMOLTLV_TYPE_BYTES:  # Bytes
            return data
        
        if type_id == SMOLTLV_TYPE_STRING:  # String
            return data.decode('utf-8')

        if self.allow_unknown_types:
            return UnknownTLV(type_id, data)
        
        raise DecoderError(f"Unknown type ID: {type_id:02x}")

class Encoder:
    def __init__(self, fp):
        self.fp = fp

    def _write_header(self, type_id, length):
        header = (type_id << 24) | (length & 0xFFFFFF)
        self.fp.write(struct.pack('>I', header))

    def encode(self, value):
        if value is None:
            self._write_header(SMOLTLV_TYPE_NULL, 0)
            return

        if value is True:
            self._write_header(SMOLTLV_TYPE_BOOL_TRUE, 0)
            return

        if value is False:
            self._write_header(SMOLTLV_TYPE_BOOL_FALSE, 0)
            return

        if isinstance(value, int):
            self._write_header(SMOLTLV_TYPE_INT, 8)
            self.fp.write(struct.pack('>q', value))
            return

        if isinstance(value, bytes):
            self._write_header(SMOLTLV_TYPE_BYTES, len(value))
            self.fp.write(value)
            return

        if isinstance(value, str):
            encoded_str = value.encode('utf-8')
            self._write_header(SMOLTLV_TYPE_STRING, len(encoded_str))
            self.fp.write(encoded_str)
            return

        if isinstance(value, list):
            buffer = BytesIO()
            encoder = Encoder(buffer)
            for item in value:
                encoder.encode(item)
            data = buffer.getvalue()
            self._write_header(SMOLTLV_TYPE_LIST, len(data))
            self.fp.write(data)
            return

        if isinstance(value, dict):
            buffer = BytesIO()
            encoder = Encoder(buffer)
            for key, val in value.items():
                encoder.encode(key)
                encoder.encode(val)
            data = buffer.getvalue()
            self._write_header(SMOLTLV_TYPE_DICT, len(data))
            self.fp.write(data)
            return

        if isinstance(value, UnknownTLV):
            self._write_header(value.type_id, len(value.data))
            self.fp.write(value.data)
            return

        raise ValueError(f"Unsupported type: {type(value)}")
    
def loads(data, allow_unknown_types=False):
    decoder = Decoder(BytesIO(data), allow_unknown_types=allow_unknown_types)
    return decoder.decode()

def dumps(value):
    buffer = BytesIO()
    encoder = Encoder(buffer)
    encoder.encode(value)
    return buffer.getvalue()

__all__ = [
    "Decoder",
    "Encoder",
    "loads",
    "dumps",
    "DecoderError",
    "UnknownTLV",
]
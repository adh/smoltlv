import smoltlv

def hexdump(data: bytes):
    def to_printable_ascii(byte):
        return chr(byte) if 32 <= byte <= 126 else "."
    res = ""
    offset = 0
    while offset < len(data):
        chunk = data[offset : offset + 16]
        hex_values = " ".join(f"{byte:02x}" for byte in chunk)
        ascii_values = "".join(to_printable_ascii(byte) for byte in chunk)
        res += (f"{offset:08x}  {hex_values:<48}  |{ascii_values}|\n")
        offset += 16

    return res


def try_roundtrip(value, allow_unknown_types=False):
    print("Testing value:", repr(value))

    encoded = smoltlv.dumps(value)

    print("Encoded data:")
    print(hexdump(encoded))

    decoded = smoltlv.loads(encoded, allow_unknown_types=allow_unknown_types)
    assert decoded == value, f"Roundtrip failed: {decoded} != {value}"


try_roundtrip(None)
try_roundtrip(True)
try_roundtrip(False)
try_roundtrip(42)
try_roundtrip(-1)
try_roundtrip(b"hello world")
try_roundtrip("hello world")
try_roundtrip([1, 2, 3, "four", True, None])
try_roundtrip({"key1": "value1", "key2": 12345, "key3": False})
try_roundtrip({})
try_roundtrip([])

try_roundtrip({"nested": {"list": [1, 2, 3], "bool": True}, "empty_dict": {}, "empty_list": []})

try_roundtrip(smoltlv.UnknownTLV(0x99, b"\x01\x02\x03\x04"), allow_unknown_types=True)
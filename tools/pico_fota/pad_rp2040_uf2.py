import struct
import sys
from pathlib import Path

BLOCK_SIZE = 512
PAGE_SIZE = 256
SECTOR_SIZE = 4096
FLASH_START = 0x10000000
FLASH_END = 0x10200000
RP2040_FAMILY = 0xE48BFF56
FAMILY_ID_FLAG = 0x2000


def pad_uf2(image):
    if not image or len(image) % BLOCK_SIZE:
        raise ValueError("Expected a non-empty UF2 with complete blocks")

    pages = {}
    for offset in range(0, len(image), BLOCK_SIZE):
        block = bytearray(image[offset:offset + BLOCK_SIZE])
        magic0, magic1, flags, address, size, _, _, family = struct.unpack_from(
            "<8I", block
        )
        end_magic, = struct.unpack_from("<I", block, BLOCK_SIZE - 4)
        if (
            (magic0, magic1, end_magic) != (0x0A324655, 0x9E5D5157, 0x0AB16F30)
            or flags != FAMILY_ID_FLAG
            or family != RP2040_FAMILY
            or size != PAGE_SIZE
            or address % PAGE_SIZE
            or not FLASH_START <= address < FLASH_END
            or address in pages
        ):
            raise ValueError(f"Unexpected or duplicate UF2 page at {address:#010x}")
        pages[address] = block

    sectors = sorted({address // SECTOR_SIZE for address in pages})
    template = pages[min(pages)]
    for sector in sectors[:-1]:
        start = sector * SECTOR_SIZE
        for address in range(start, start + SECTOR_SIZE, PAGE_SIZE):
            if address in pages:
                continue
            block = bytearray(template)
            struct.pack_into("<I", block, 12, address)
            block[32:32 + PAGE_SIZE] = b"\xff" * PAGE_SIZE
            pages[address] = block

    output = bytearray()
    for number, address in enumerate(sorted(pages)):
        block = pages[address]
        struct.pack_into("<II", block, 20, number, len(pages))
        output.extend(block)
    return output


if __name__ == "__main__":
    source, destination = map(Path, sys.argv[1:])
    destination.write_bytes(pad_uf2(source.read_bytes()))

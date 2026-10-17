#!/usr/bin/env python3
"""Build a minimal, unencrypted GameCube GCM image for one homebrew DOL."""

import argparse
from datetime import date
import struct
from pathlib import Path

DISC_HEADER_SIZE = 0x440
BI2_OFFSET = DISC_HEADER_SIZE
BI2_SIZE = 0x2000
APPLOADER_OFFSET = BI2_OFFSET + BI2_SIZE
APPLOADER_HEADER_SIZE = 0x20
DOL_HEADER_SIZE = 0x100
APPLOADER_LOAD_ADDRESS = 0x81200000
APPLOADER_LIMIT = 0x00100000
FST_LOAD_TOP = 0x81800000
ALIGNMENT = 0x20
DISC_IMAGE_ALIGNMENT = 0x8000
GC_MAGIC = 0xC2339F3D


def align(value, boundary=ALIGNMENT):
    return (value + boundary - 1) & ~(boundary - 1)


def read_u32(data, offset):
    return struct.unpack_from(">I", data, offset)[0]


def write_u32(data, offset, value):
    struct.pack_into(">I", data, offset, value)


def validate_dol(dol):
    if len(dol) < DOL_HEADER_SIZE:
        raise ValueError("DOL is shorter than its 0x100-byte header")

    sections = []
    for index in range(18):
        file_offset = read_u32(dol, index * 4)
        address = read_u32(dol, 0x48 + index * 4)
        size = read_u32(dol, 0x90 + index * 4)
        if size == 0:
            continue
        if file_offset < DOL_HEADER_SIZE or file_offset + size > len(dol):
            raise ValueError(f"DOL section {index} extends outside the file")
        if address < 0x80000000 or address + size > 0x81800000:
            raise ValueError(f"DOL section {index} has an invalid load address")
        sections.append((address, address + size))

    entry_point = read_u32(dol, 0xe0)
    if not any(start <= entry_point < end for start, end in sections):
        raise ValueError("DOL entry point is not inside a loaded section")
    return sections


def make_fst(dol_offset, dol_size):
    filename = b"GLITCHCRAFT.DOL\0"
    root = struct.pack(">III", 0x01000000, 0, 2)
    dol_entry = struct.pack(">III", 1, dol_offset, dol_size)
    fst = root + dol_entry + b"\0" + filename
    return fst


def build_image(dol, apploader, title):
    sections = validate_dol(dol)
    if not apploader or len(apploader) > APPLOADER_LIMIT:
        raise ValueError("apploader must be nonempty and at most 1 MiB")

    app_entry = APPLOADER_LOAD_ADDRESS
    app_header = (
        date.today().strftime("%Y/%m/%d").encode("ascii").ljust(16, b"\0")
        + struct.pack(">III", app_entry, len(apploader), 0)
        + b"\0" * 4
    )
    dol_offset = align(
        APPLOADER_OFFSET + APPLOADER_HEADER_SIZE + len(apploader)
    )
    fst_offset = align(dol_offset + len(dol))
    fst = make_fst(dol_offset, len(dol))
    fst_destination = (FST_LOAD_TOP - len(fst)) & ~(ALIGNMENT - 1)
    app_end = APPLOADER_LOAD_ADDRESS + APPLOADER_LIMIT
    if any(
        address < app_end and end > APPLOADER_LOAD_ADDRESS
        for address, end in sections
    ):
        raise ValueError("DOL load section overlaps the apploader")
    if any(
        address < FST_LOAD_TOP and end > fst_destination
        for address, end in sections
    ):
        raise ValueError("DOL load section overlaps the filesystem load area")

    header = bytearray(DISC_HEADER_SIZE)
    header[0:6] = b"GCRE01"
    header[8] = 0
    header[9] = 0
    write_u32(header, 0x1c, GC_MAGIC)
    encoded_title = title.encode("ascii", errors="strict")[:0x60]
    header[0x20 : 0x20 + len(encoded_title)] = encoded_title
    write_u32(header, 0x420, dol_offset)
    write_u32(header, 0x424, fst_offset)
    write_u32(header, 0x428, len(fst))
    write_u32(header, 0x42c, len(fst))
    bi2 = bytearray(BI2_SIZE)
    write_u32(bi2, 0x18, 1)

    image_size = align(fst_offset + len(fst), DISC_IMAGE_ALIGNMENT)
    image = bytearray(image_size)
    image[:DISC_HEADER_SIZE] = header
    image[BI2_OFFSET : BI2_OFFSET + BI2_SIZE] = bi2
    app_start = APPLOADER_OFFSET
    image[app_start : app_start + len(app_header)] = app_header
    app_code_start = app_start + APPLOADER_HEADER_SIZE
    image[app_code_start : app_code_start + len(apploader)] = apploader
    image[dol_offset : dol_offset + len(dol)] = dol
    image[fst_offset : fst_offset + len(fst)] = fst
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", type=Path, required=True)
    parser.add_argument("--apploader", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--title", default="GlitchCraft GameCube Edition")
    args = parser.parse_args()

    dol = args.dol.read_bytes()
    apploader = args.apploader.read_bytes()
    image = build_image(dol, apploader, args.title)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(image)
    print(f"Wrote {args.output} ({len(image)} bytes)")


if __name__ == "__main__":
    main()

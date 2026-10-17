import importlib.util
import struct
import sys
import unittest
from pathlib import Path


sys.dont_write_bytecode = True
SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "make-gamecube-image.py"
SPEC = importlib.util.spec_from_file_location("make_gamecube_image", SCRIPT)
IMAGE_BUILDER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMAGE_BUILDER)


def make_test_dol():
    dol = bytearray(0x120)
    struct.pack_into(">I", dol, 0, 0x100)
    struct.pack_into(">I", dol, 0x48, 0x80004000)
    struct.pack_into(">I", dol, 0x90, 0x20)
    struct.pack_into(">I", dol, 0xe0, 0x80004000)
    dol[0x100:0x120] = bytes(range(0x20))
    return bytes(dol)


class GameCubeImageTests(unittest.TestCase):
    def test_builds_disc_header_apploader_dol_and_fst(self):
        dol = make_test_dol()
        apploader = bytes(range(64))
        image = IMAGE_BUILDER.build_image(
            dol, apploader, "GlitchCraft test image"
        )

        self.assertEqual(len(image) % IMAGE_BUILDER.DISC_IMAGE_ALIGNMENT, 0)
        self.assertEqual(IMAGE_BUILDER.read_u32(image, 0x1c),
                         IMAGE_BUILDER.GC_MAGIC)
        self.assertEqual(image[:6], b"GCRE01")
        self.assertEqual(IMAGE_BUILDER.read_u32(image, 0x458), 1)

        dol_offset = IMAGE_BUILDER.read_u32(image, 0x420)
        fst_offset = IMAGE_BUILDER.read_u32(image, 0x424)
        fst_size = IMAGE_BUILDER.read_u32(image, 0x428)
        self.assertEqual(dol_offset % IMAGE_BUILDER.ALIGNMENT, 0)
        self.assertEqual(fst_offset % IMAGE_BUILDER.ALIGNMENT, 0)
        self.assertEqual(image[dol_offset:dol_offset + len(dol)], dol)

        app_header = IMAGE_BUILDER.APPLOADER_OFFSET
        self.assertEqual(
            IMAGE_BUILDER.read_u32(image, app_header + 0x10),
            IMAGE_BUILDER.APPLOADER_LOAD_ADDRESS,
        )
        self.assertEqual(
            IMAGE_BUILDER.read_u32(image, app_header + 0x14),
            len(apploader),
        )
        self.assertEqual(
            image[app_header + 0x20:app_header + 0x20 + len(apploader)],
            apploader,
        )

        fst = image[fst_offset:fst_offset + fst_size]
        self.assertEqual(struct.unpack_from(">III", fst, 0),
                         (0x01000000, 0, 2))
        self.assertEqual(struct.unpack_from(">III", fst, 12),
                         (1, dol_offset, len(dol)))
        self.assertIn(b"\0GLITCHCRAFT.DOL\0", fst[24:])

    def test_rejects_invalid_dol_section_and_entrypoint(self):
        dol = bytearray(make_test_dol())
        struct.pack_into(">I", dol, 0, len(dol) - 4)
        with self.assertRaisesRegex(ValueError, "extends outside"):
            IMAGE_BUILDER.build_image(bytes(dol), b"apploader", "test")

        dol = bytearray(make_test_dol())
        struct.pack_into(">I", dol, 0xe0, 0x80005000)
        with self.assertRaisesRegex(ValueError, "entry point"):
            IMAGE_BUILDER.build_image(bytes(dol), b"apploader", "test")

    def test_rejects_dol_section_overlapping_apploader_memory(self):
        dol = bytearray(make_test_dol())
        struct.pack_into(">I", dol, 0x48, IMAGE_BUILDER.APPLOADER_LOAD_ADDRESS)
        struct.pack_into(">I", dol, 0xe0, IMAGE_BUILDER.APPLOADER_LOAD_ADDRESS)
        with self.assertRaisesRegex(ValueError, "overlaps the apploader"):
            IMAGE_BUILDER.build_image(bytes(dol), b"apploader", "test")


if __name__ == "__main__":
    unittest.main()

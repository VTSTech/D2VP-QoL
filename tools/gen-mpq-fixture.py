#!/usr/bin/env python3
"""Generate platform/linux/tests/d2vp_smoke.mpq - the d2linux M2 smoke MPQ fixture.

A minimal but structurally complete MPQ v1 archive that exercises the vendored
Storm's real open path (Storm/Source/SFile.cpp):

  header at 0x200 (the scan steps 0x200 and checks dwMagic/dwHeaderSize)
  hash table 4 entries, encrypted with SFILE_Hash(3, "(hash table)")
  block table 1 entry,  encrypted with SFILE_Hash(3, "(block table)")
  one stored file "d2vp_fix.txt" (EXISTS|SINGLE_UNIT, contiguous, 512 bytes)

The crypt/hash routines below are transcribed EXACTLY from the decomp
(SFILE_InitializeHashSource / SFILE_Hash / SFILE_Decrypt) so the fixture is
ground-truth against the code that reads it - not against a third-party lib.
mpyq (independent MPQ reader) additionally validates the container shape.

Usage: python3 tools/gen-mpq-fixture.py [outfile]
"""
import struct
import sys

HEADER_POS = 0x200
FILE_NAME = "d2vp_fix.txt"
PAYLOAD_MAGIC = b"D2VP-MPQ-SMOKE-FIXTURE-1\n"
PAYLOAD_SIZE = 512
MPQ_FILE_EXISTS = 0x80000000
MPQ_FILE_SINGLE_UNIT = 0x01000000


def build_crypt_table():
    """SFILE_InitializeHashSource, verbatim semantics (column-major fill)."""
    table = [0] * 0x500
    seed = 0x00100001
    for index in range(0x100):
        for j in range(5):
            seed = (seed * 0x7D + 3) % 0x2AAAAB
            hi = (seed & 0xFFFF) << 0x10
            seed = (seed * 0x7D + 3) % 0x2AAAAB
            lo = seed & 0xFFFF
            table[index + j * 0x100] = (hi | lo) & 0xFFFFFFFF
    return table


CRYPT = build_crypt_table()


def mpq_hash(kind, text):
    """SFILE_Hash, verbatim semantics."""
    n_hash = 0x7FED7FED
    n_seed = 0xEEEEEEEE  # the decomp's -0x11111112
    for ch in text.upper().encode("ascii"):
        n_hash = ((n_hash + n_seed) ^ CRYPT[kind * 0x100 + (ch & 0xFF)]) & 0xFFFFFFFF
        n_seed = (n_seed * 0x21 + (ch & 0xFF) + 3 + n_hash) & 0xFFFFFFFF
    return n_hash


def mpq_encrypt(dwords, key):
    """The inverse of SFILE_Decrypt.

    The decomp's decrypt updates its key with the DECRYPTED dword
    (nKey = nKey * 0x21 + 3 + nDecrypted), so encryption replays the same
    chain using the known plaintext and XORs out (nKey + nSeed) per dword.
    """
    n_key = 0xEEEEEEEE
    out = []
    for plain in dwords:
        n_key = (n_key + CRYPT[0x400 + (key & 0xFF)]) & 0xFFFFFFFF
        cipher = plain ^ ((n_key + key) & 0xFFFFFFFF)
        out.append(cipher)
        key = (((~key & 0xFFFFFFFF) << 0x15) + 0x11111111 | (key >> 0x0B)) & 0xFFFFFFFF
        n_key = (n_key * 0x21 + 3 + plain) & 0xFFFFFFFF
    return out


def build_payload():
    payload = bytearray(PAYLOAD_SIZE)
    payload[0 : len(PAYLOAD_MAGIC)] = PAYLOAD_MAGIC
    # Deterministic filler: the smoke asserts both the magic and the pattern.
    for i in range(len(PAYLOAD_MAGIC), PAYLOAD_SIZE):
        payload[i] = (i * 7 + 3) & 0xFF
    return bytes(payload)


def build_mpq():
    payload = build_payload()

    # Header-relative layout: header | hash table (4) | block table (1) | data
    hash_pos = 0x20
    block_pos = hash_pos + 4 * 16
    data_pos = block_pos + 16
    archive_size = data_pos + len(payload)

    # Block table (plaintext): one entry, 16 bytes, positions header-relative.
    block_entry = struct.pack(
        "<IIII",
        data_pos,                      # dwFilePos
        len(payload),                  # dwCompressedSize
        len(payload),                  # dwFileSize
        MPQ_FILE_EXISTS | MPQ_FILE_SINGLE_UNIT,
    )
    block_dwords = list(struct.unpack("<4I", block_entry))
    block_key = mpq_hash(3, "(block table)")
    block_enc = struct.pack("<4I", *mpq_encrypt(block_dwords, block_key))

    # Hash table (plaintext): 4 slots; file in its natural slot, rest FREE.
    start_slot = mpq_hash(0, FILE_NAME) & 3
    slots = []
    for i in range(4):
        if i == start_slot:
            slots.append(
                struct.pack(
                    "<IIHHI",
                    mpq_hash(1, FILE_NAME),
                    mpq_hash(2, FILE_NAME),
                    0,  # wLocale
                    0,  # wPlatform
                    0,  # dwBlockIndex
                )
            )
        else:
            slots.append(struct.pack("<IIHHI", 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFFFFFFFF))
    hash_dwords = list(struct.unpack("<16I", b"".join(slots)))
    hash_key = mpq_hash(3, "(hash table)")
    hash_enc = struct.pack("<16I", *mpq_encrypt(hash_dwords, hash_key))

    # Header field order per the vendored Storm's D2MPQHeaderStrc (and mpyq):
    #   0x00 magic, 0x04 header size, 0x08 archive size,
    #   0x0C format version (u16), 0x0E sector shift (u16),
    #   0x10 hash table POS, 0x14 block table POS,
    #   0x18 hash table SIZE, 0x1C block table SIZE.
    # (The classic-zed order pos/size/pos/size is NOT what this Storm reads.)
    header = struct.pack(
        "<IIIHHIIII",
        0x1A51504D,        # 'MPQ\x1a'
        0x20,              # dwHeaderSize
        archive_size,      # dwArchiveSize (past the header)
        0,                 # wFormatVersion (v1)
        3,                 # wBlockSize: 0x200 << 3 = 4096-byte sectors
        hash_pos,          # dwHashTablePos (header-relative)
        block_pos,         # dwBlockTablePos (header-relative)
        4,                 # dwHashTableSize (entries)
        1,                 # dwBlockTableSize (entries)
    )
    assert len(header) == 0x20

    # Blizzard-style user-data header at 0 (what retail MPQs carry; the
    # decomp's scan skips it - magic 'MPQ\x1b' != 'MPQ\x1a' - and finds the
    # real header at +0x200; mpyq follows the pointer).
    userdata = struct.pack(
        "<IIII",
        0x1B51504D,    # 'MPQ\x1b'
        0,             # UserDataSize
        HEADER_POS,    # mpq_header_offset
        0x10,          # UserDataHeaderSize
    )

    out = bytearray(HEADER_POS + archive_size)
    out[0:0x10] = userdata
    out[HEADER_POS : HEADER_POS + 0x20] = header
    out[HEADER_POS + hash_pos : HEADER_POS + hash_pos + len(hash_enc)] = hash_enc
    out[HEADER_POS + block_pos : HEADER_POS + block_pos + len(block_enc)] = block_enc
    out[HEADER_POS + data_pos : HEADER_POS + data_pos + len(payload)] = payload
    return bytes(out)


def self_check(blob):
    """Re-open the built blob with the decomp's own algorithm (in reverse)."""
    header = struct.unpack("<IIIHHIIII", blob[HEADER_POS : HEADER_POS + 0x20])
    magic, hdr_size, arch_size, ver, blk_shift, hash_pos, block_pos, hash_n, block_n = header
    assert magic == 0x1A51504D and hdr_size >= 0x20
    # Decrypt the hash table with the decomp's SFILE_Decrypt semantics.
    def decrypt(dwords, key):
        n_key = 0xEEEEEEEE
        out = []
        for data in dwords:
            n_key = (n_key + CRYPT[0x400 + (key & 0xFF)]) & 0xFFFFFFFF
            plain = data ^ ((n_key + key) & 0xFFFFFFFF)
            out.append(plain)
            key = (((~key & 0xFFFFFFFF) << 0x15) + 0x11111111 | (key >> 0x0B)) & 0xFFFFFFFF
            n_key = (n_key * 0x21 + 3 + plain) & 0xFFFFFFFF
        return out

    raw = struct.unpack("<%dI" % (hash_n * 4), blob[HEADER_POS + hash_pos : HEADER_POS + hash_pos + hash_n * 16])
    table = struct.pack("<%dI" % (hash_n * 4), *decrypt(list(raw), mpq_hash(3, "(hash table)")))
    slot = mpq_hash(0, FILE_NAME) & (hash_n - 1)
    name_a, name_b, _loc, _plat, block_idx = struct.unpack_from("<IIHHI", table, slot * 16)
    assert name_a == mpq_hash(1, FILE_NAME), "hash A mismatch"
    assert name_b == mpq_hash(2, FILE_NAME), "hash B mismatch"
    assert block_idx == 0, "block index mismatch"
    raw = struct.unpack("<4I", blob[HEADER_POS + block_pos : HEADER_POS + block_pos + 16])
    fpos, csize, fsize, flags = struct.unpack("<4I", struct.pack("<4I", *decrypt(list(raw), mpq_hash(3, "(block table)"))))
    assert fpos == 0x70 and csize == PAYLOAD_SIZE and fsize == PAYLOAD_SIZE, (fpos, csize, fsize)
    assert flags == (MPQ_FILE_EXISTS | MPQ_FILE_SINGLE_UNIT), hex(flags)
    assert blob[HEADER_POS + fpos : HEADER_POS + fpos + len(PAYLOAD_MAGIC)] == PAYLOAD_MAGIC
    print(
        "self-check OK: slot=%d filepos=0x%x csize=%d fsize=%d flags=0x%08x total=%d bytes"
        % (slot, fpos, csize, fsize, flags, len(blob))
    )


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else "platform/linux/tests/d2vp_smoke.mpq"
    blob = build_mpq()
    self_check(blob)
    with open(out_path, "wb") as f:
        f.write(blob)
    print("wrote %s (%d bytes)" % (out_path, len(blob)))


if __name__ == "__main__":
    main()

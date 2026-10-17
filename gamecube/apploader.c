typedef unsigned int u32;
typedef unsigned char u8;

typedef void (*AppInit)(void);
typedef int (*AppMain)(u32 *, u32 *, u32 *);
typedef u32 (*AppClose)(void);

static volatile u32 app_stage;
static volatile u32 dol_offset;
static volatile u32 fst_offset;
static volatile u32 fst_size;
static volatile u32 fst_max_size;
static volatile u32 section_index;
static volatile u32 dol_header_saved;
static volatile u32 fst_loaded;
static volatile u32 entry_point;
static volatile u32 fst_destination;
static volatile u8 dol_header[0x100];

static u32 read_be32(const volatile u8 *bytes)
{
    return ((u32)bytes[0] << 24) | ((u32)bytes[1] << 16) |
           ((u32)bytes[2] << 8) | (u32)bytes[3];
}

static void write_be32(volatile u32 *address, u32 value)
{
    *address = value;
}

static void app_init(void)
{
    app_stage = 0;
    dol_offset = 0;
    fst_offset = 0;
    fst_size = 0;
    fst_max_size = 0;
    section_index = 0;
    dol_header_saved = 0;
    fst_loaded = 0;
    entry_point = 0;
    fst_destination = 0;
    for (u32 index = 0; index < 0x100; ++index) {
        dol_header[index] = 0;
    }
}

static int app_main(u32 *destination, u32 *length, u32 *disc_offset)
{
    volatile u8 *scratch = (volatile u8 *)0x80004000;

    if (app_stage == 0) {
        *destination = (u32)scratch;
        *length = 0x440;
        *disc_offset = 0;
        app_stage = 1;
        return 1;
    }

    if (app_stage == 1) {
        dol_offset = read_be32(scratch + 0x420);
        fst_offset = read_be32(scratch + 0x424);
        fst_size = read_be32(scratch + 0x428);
        fst_max_size = read_be32(scratch + 0x42c);
        *destination = (u32)scratch;
        *length = 0x100;
        *disc_offset = dol_offset;
        app_stage = 2;
        return 1;
    }

    if (app_stage == 2) {
        if (!dol_header_saved) {
            for (u32 index = 0; index < 0x100; ++index) {
                dol_header[index] = scratch[index];
            }
            dol_header_saved = 1;
        }
        while (section_index < 18) {
            const u32 index = section_index++;
            const u32 file_offset = read_be32(dol_header + index * 4);
            const u32 address = read_be32(dol_header + 0x48 + index * 4);
            const u32 section_size =
                read_be32(dol_header + 0x90 + index * 4);
            if (file_offset == 0 || section_size == 0) {
                continue;
            }
            *destination = address;
            *length = section_size;
            *disc_offset = dol_offset + file_offset;
            return 1;
        }
        entry_point = read_be32(dol_header + 0xe0);
        app_stage = 3;
    }

    if (app_stage == 3 && !fst_loaded && fst_size != 0) {
        fst_destination = (0x81800000 - fst_size) & ~31U;
        *destination = fst_destination;
        *length = fst_size;
        *disc_offset = fst_offset;
        write_be32((volatile u32 *)0x80000038, fst_destination);
        write_be32((volatile u32 *)0x8000003c, fst_size);
        write_be32((volatile u32 *)0x80000040, fst_max_size);
        fst_loaded = 1;
        return 1;
    }

    return 0;
}

static u32 app_close(void)
{
    return entry_point;
}

void apploader_entry(AppInit *init, AppMain *main, AppClose *close)
{
    *init = app_init;
    *main = app_main;
    *close = app_close;
}

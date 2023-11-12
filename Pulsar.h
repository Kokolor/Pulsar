typedef struct
{
        unsigned char magic[2];
        unsigned char mode;
        unsigned char charsize;
} PSF1_HEADER;

typedef struct
{
        PSF1_HEADER *psf1_Header;
        void *glyphBuffer;
} PSF1_FONT;

typedef struct
{
        uint32_t type;
        uintptr_t phys_addr;
        uintptr_t virt_addr;
        uint64_t num_pages;
        uint64_t attrib;
} EFI_MEMORY_DESCRIPTOR;

typedef struct
{
        unsigned int *framebuffer;
        unsigned int width;
        unsigned int height;
        unsigned int pitch;
        unsigned int pixel_per_scanline;
        EFI_MEMORY_DESCRIPTOR *mem_map;
        uint64_t mem_map_size;
        uint64_t mem_map_desc_size;
        uint8_t debug_level;
        int argc;
        char **argv;
        PSF1_FONT *font;
} bootinfo_t;

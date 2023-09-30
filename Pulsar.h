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
        unsigned int *framebuffer;
        unsigned int width;
        unsigned int height;
        unsigned int pitch;
        unsigned int pixel_per_scanline;
        int argc;
        char **argv;
        PSF1_FONT *font;
} bootinfo_t;

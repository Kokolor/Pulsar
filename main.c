#include <uefi.h>
#include <string.h>
#include "Pulsar.h"

// Thanks to https://gitlab.com/bztsrc/posix-uefi/-/blob/master/examples/

uint8_t debug_on = 1;

/*** ELF64 defines and structs ***/
#define ELFMAG "\177ELF"
#define SELFMAG 4
#define EI_CLASS 4    /* File class byte index */
#define ELFCLASS64 2  /* 64-bit objects */
#define EI_DATA 5     /* Data encoding byte index */
#define ELFDATA2LSB 1 /* 2's complement, little endian */
#define ET_EXEC 2     /* Executable file */
#define PT_LOAD 1     /* Loadable program segment */
#ifdef __x86_64__
#define EM_MACH 62 /* AMD x86-64 architecture */
#endif
#ifdef __aarch64__
#define EM_MACH 183 /* ARM aarch64 architecture */
#endif

uintptr_t entry;

typedef struct
{
  uint8_t e_ident[16];  /* Magic number and other info */
  uint16_t e_type;      /* Object file type */
  uint16_t e_machine;   /* Architecture */
  uint32_t e_version;   /* Object file version */
  uint64_t e_entry;     /* Entry point virtual address */
  uint64_t e_phoff;     /* Program header table file offset */
  uint64_t e_shoff;     /* Section header table file offset */
  uint32_t e_flags;     /* Processor-specific flags */
  uint16_t e_ehsize;    /* ELF header size in bytes */
  uint16_t e_phentsize; /* Program header table entry size */
  uint16_t e_phnum;     /* Program header table entry count */
  uint16_t e_shentsize; /* Section header table entry size */
  uint16_t e_shnum;     /* Section header table entry count */
  uint16_t e_shstrndx;  /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
  uint32_t p_type;   /* Segment type */
  uint32_t p_flags;  /* Segment flags */
  uint64_t p_offset; /* Segment file offset */
  uint64_t p_vaddr;  /* Segment virtual address */
  uint64_t p_paddr;  /* Segment physical address */
  uint64_t p_filesz; /* Segment size in file */
  uint64_t p_memsz;  /* Segment size in memory */
  uint64_t p_align;  /* Segment alignment */
} Elf64_Phdr;

PSF1_FONT *load_font(const char *path)
{
  FILE *file;
  PSF1_FONT *font = NULL;
  PSF1_HEADER header;

  file = fopen(path, "r");
  if (file == NULL)
  {
    return NULL;
  }

  fread(&header, sizeof(PSF1_HEADER), 1, file);

  if (header.magic[0] != 0x36 || header.magic[1] != 0x04)
  {
    fclose(file);
    return NULL;
  }

  font = malloc(sizeof(PSF1_FONT));
  if (font == NULL)
  {
    fclose(file);
    return NULL;
  }

  font->glyphBuffer = malloc(header.charsize * 256);
  if (font->glyphBuffer == NULL)
  {
    free(font);
    fclose(file);
    return NULL;
  }

  fread(font->glyphBuffer, header.charsize, 256, file);
  fclose(file);

  font->psf1_Header = &header;

  font->psf1_Header = malloc(sizeof(PSF1_HEADER));
  if (font->psf1_Header == NULL)
  {
    free(font->glyphBuffer);
    free(font);
    fclose(file);
    return NULL;
  }
  memcpy(font->psf1_Header, &header, sizeof(PSF1_HEADER));

  return font;
}

void fill_memory_map(bootinfo_t *bootinfo)
{
  efi_status_t status;
  efi_memory_descriptor_t *efi_mem_map = NULL;
  uintn_t mem_map_size = 0, map_key = 0, desc_size = 0;

  status = BS->GetMemoryMap(&mem_map_size, NULL, &map_key, &desc_size, NULL);
  if (status != EFI_BUFFER_TOO_SMALL || !mem_map_size)
  {
    return;
  }

  mem_map_size += 4 * desc_size;
  efi_mem_map = (efi_memory_descriptor_t *)malloc(mem_map_size);
  if (!efi_mem_map)
  {
    return;
  }

  status = BS->GetMemoryMap(&mem_map_size, efi_mem_map, &map_key, &desc_size, NULL);
  if (EFI_ERROR(status))
  {
    return;
  }

  bootinfo->mem_map = (EFI_MEMORY_DESCRIPTOR *)efi_mem_map;
  bootinfo->mem_map_size = mem_map_size;
  bootinfo->mem_map_desc_size = desc_size;
}

void load_elf(char *filename)
{
  FILE *f;
  char *buff;
  long int size;
  Elf64_Ehdr *elf;
  Elf64_Phdr *phdr;
  uintptr_t entry;
  efi_status_t status;
  bootinfo_t bootinfo;
  efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  efi_gop_t *gop = NULL;
  int i;

  if ((f = fopen(filename, "r")))
  {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buff = malloc(size + 1);
    if (!buff)
    {
      fprintf(stderr, "Unable to allocate memory\n");
      while (1)
        ;
    }
    fread(buff, size, 1, f);
    fclose(f);
  }
  else
  {
    fprintf(stderr, "Unable to open file\n");
    while (1)
      ;
  }

  memset(&bootinfo, 0, sizeof(bootinfo_t));
  status = BS->LocateProtocol(&gopGuid, NULL, (void **)&gop);
  if (!EFI_ERROR(status) && gop)
  {
    status = gop->SetMode(gop, 0);
    ST->ConOut->Reset(ST->ConOut, 0);
    ST->StdErr->Reset(ST->StdErr, 0);
    if (EFI_ERROR(status))
    {
      fprintf(stderr, "Unable to set video mode\n");
      while (1)
        ;
    }

    fill_memory_map(&bootinfo);

    bootinfo.framebuffer = (unsigned int *)gop->Mode->FrameBufferBase;
    bootinfo.width = gop->Mode->Information->HorizontalResolution;
    bootinfo.height = gop->Mode->Information->VerticalResolution;
    bootinfo.pitch = sizeof(unsigned int) * gop->Mode->Information->PixelsPerScanLine;
    bootinfo.pixel_per_scanline = gop->Mode->Information->PixelsPerScanLine;
    bootinfo.font = load_font("\\EFI\\BOOT\\zap-light16.psf");
    if (bootinfo.font == NULL)
    {
      printf("Font is not valid or is not found\n");
    }
    else
    {
      printf("Font found: char size = %d\n", bootinfo.font->psf1_Header->charsize);
    }
  }
  else
  {
    fprintf(stderr, "unable to get graphics output protocol\n");
    while (1)
      ;
  }

  elf = (Elf64_Ehdr *)buff;
  if (!memcmp(elf->e_ident, ELFMAG, SELFMAG) && /* magic match? */
      elf->e_ident[EI_CLASS] == ELFCLASS64 &&   /* 64 bit? */
      elf->e_ident[EI_DATA] == ELFDATA2LSB &&   /* LSB? */
      elf->e_type == ET_EXEC &&                 /* executable object? */
      elf->e_machine == EM_MACH &&              /* architecture match? */
      elf->e_phnum > 0)
  {
    for (phdr = (Elf64_Phdr *)(buff + elf->e_phoff), i = 0;
         i < elf->e_phnum;
         i++, phdr = (Elf64_Phdr *)((uint8_t *)phdr + elf->e_phentsize))
    {
      if (phdr->p_type == PT_LOAD)
      {
        memcpy((void *)phdr->p_vaddr, buff + phdr->p_offset, phdr->p_filesz);
        memset((void *)(phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
      }
    }
    entry = elf->e_entry;
  }
  else
  {
    fprintf(stderr, "not a valid ELF executable for this architecture\n");
    while (1)
      ;
  }
  free(buff);

  if (exit_bs())
  {
    fprintf(stderr,
            "Ph'nglui mglw'nafh Chtulu R'lyeh wgah'nagl fhtagn\n"
            "(Hastur has a hold on us and won't let us go)\n");
    while (1)
      ;
  }

  (*((void (*__attribute__((sysv_abi)))(bootinfo_t *))(entry)))(&bootinfo);
  while (1)
    ;
}

void display_menu(char *files[], int file_count, int selected)
{
  for (int i = 0; i < file_count; i++)
  {
    if (i == selected)
    {
      printf("-> %s\n", files[i]);
    }
    else
    {
      printf("   %s\n", files[i]);
    }
  }
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  DIR *dir;
  struct dirent *ent;
  char *ext;
  char *files[100];
  int file_count = 0;
  int selected = 0;
  efi_input_key_t key;
  efi_status_t status;

  ST->ConOut->ClearScreen(ST->ConOut);

  if ((dir = opendir("\\EFI\\BOOT")) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      ext = strrchr(ent->d_name, '.');
      if (ext != NULL)
      {
        if (strcmp(ext, ".bin") == 0 || strcmp(ext, ".elf") == 0)
        {
          files[file_count] = strdup(ent->d_name);
          file_count++;
        }
      }
    }
    closedir(dir);
  }
  else
  {
    fprintf(stderr, "Sorry, i can't open the directory!\n");
    while (1)
      ;
  }

  ST->ConOut->ClearScreen(ST->ConOut);
  display_menu(files, file_count, selected);

  while (1)
  {
    status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
    if (status == EFI_SUCCESS)
    {
      ST->ConOut->ClearScreen(ST->ConOut);

      if (key.UnicodeChar == 0)
      {
        if (key.ScanCode == 0x01)
        {
          if (selected > 0)
            selected--;
        }
        else if (key.ScanCode == 0x02)
        {
          if (selected < file_count - 1)
            selected++;
        }
      }
      else if (key.UnicodeChar == '\r')
      {
        char full_path[256];
        sprintf(full_path, "\\EFI\\BOOT\\%s", files[selected]);
        load_elf(full_path);

        while (1)
          ;
      }

      for (int i = 0; i < file_count; i++)
      {
        if (i == selected)
        {
          printf("-> %s\n", files[i]);
        }
        else
        {
          printf("   %s\n", files[i]);
        }
      }
    }
  }

  for (int i = 0; i < file_count; i++)
  {
    free(files[i]);
  }

  return 0;
}
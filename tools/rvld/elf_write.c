#include "rvld.h"
#include <stdlib.h>
#include <string.h>

static uint64_t align_up(uint64_t x, uint64_t al) {
    if (al <= 1) return x;
    return (x + al - 1) & ~(al - 1);
}

bool write_executable(LinkerCtx *ctx, const char *out_filename) {
    FILE *f = fopen(out_filename, "wb");
    if (!f) {
        perror("rvld: fopen");
        return false;
    }

    uint64_t header_sz = sizeof(Elf64_Ehdr) + 2 * sizeof(Elf64_Phdr);

    Elf64_Ehdr ehdr = {0};
    memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
    ehdr.e_ident[EI_CLASS] = ELFCLASS64;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_RISCV;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_phoff = sizeof(Elf64_Ehdr);
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = 2; 
    ehdr.e_flags = 0x4; // EF_RISCV_FLOAT_ABI_DOUBLE

    // Entry point: find _start or main
    ehdr.e_entry = ctx->text_vaddr + header_sz; // Default if no _start
    for (int i = 0; i < ctx->global_count; i++) {
        if (strcmp(ctx->globals[i].name, "_start") == 0 || strcmp(ctx->globals[i].name, "main") == 0) {
            ehdr.e_entry = ctx->globals[i].value;
            if (strcmp(ctx->globals[i].name, "_start") == 0) break;
        }
    }

    Elf64_Phdr phdr[2] = {0};

    // Segment 1: .text (RX)
    phdr[0].p_type = PT_LOAD;
    phdr[0].p_flags = PF_R | PF_X;
    phdr[0].p_offset = 0;
    phdr[0].p_vaddr = ctx->text_vaddr;
    phdr[0].p_paddr = ctx->text_vaddr;
    phdr[0].p_filesz = ctx->text_size;
    phdr[0].p_memsz = ctx->text_size;
    phdr[0].p_align = 0x1000;

    // Segment 2: .data + .bss (RW)
    phdr[1].p_type = PT_LOAD;
    phdr[1].p_flags = PF_R | PF_W;
    phdr[1].p_offset = align_up(phdr[0].p_filesz, 0x1000);
    phdr[1].p_vaddr = ctx->data_vaddr;
    phdr[1].p_paddr = ctx->data_vaddr;
    phdr[1].p_filesz = ctx->data_size;
    phdr[1].p_memsz = ctx->data_size + ctx->bss_size;
    phdr[1].p_align = 0x1000;

    // Write Headers
    fwrite(&ehdr, sizeof(ehdr), 1, f);
    fwrite(phdr, sizeof(phdr), 1, f);

    // Write .text sections
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        for (int j = 0; j < obj->section_count; j++) {
            Section *s = &obj->sections[j];
            if (s->type == SHT_PROGBITS && (s->flags & SHF_EXECINSTR)) {
                fseek(f, (long)(s->addr - ctx->text_vaddr), SEEK_SET);
                fwrite(s->data, 1, s->size, f);
            }
        }
    }

    // Write .data sections
    for (int i = 0; i < ctx->obj_count; i++) {
        ObjectFile *obj = ctx->objs[i];
        for (int j = 0; j < obj->section_count; j++) {
            Section *s = &obj->sections[j];
            if (s->type == SHT_PROGBITS && !(s->flags & SHF_EXECINSTR) && (s->flags & SHF_ALLOC)) {
                fseek(f, (long)(phdr[1].p_offset + (s->addr - ctx->data_vaddr)), SEEK_SET);
                fwrite(s->data, 1, s->size, f);
            }
        }
    }

    fclose(f);
    return true;
}

#ifndef RVAS_ELF_OUT_H
#define RVAS_ELF_OUT_H

#include "asm.h"
#include <stdio.h>

bool rvas_write_elf64_o(const RvAsmResult *a, FILE *out);

#endif

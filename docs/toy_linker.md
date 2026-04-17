# Building a Toy Linker --- From `.o` Files to Executable

## Step 0 --- Understand What Is Inside a `.o` File

Your object files are **ELF relocatable files (ET_REL)**.

Important sections inside a `.o` file:

  Section                      Meaning
  ---------------------------- -----------------------
  `.text`                      Machine code
  `.data`                      Initialized globals
  `.bss`                       Uninitialized globals
  `.symtab`                    Symbol table
  `.rel.text` / `.rela.text`   Relocations
  `.strtab`                    Symbol names

Before writing code, run:

``` bash
readelf -a file.o
```

If you don't understand this output, stop. Your linker depends on it.

------------------------------------------------------------------------

## Step 1 --- Build an ELF Parser (Non-Negotiable)

Your linker must read ELF.

You need to parse:

### ELF Header

Gives offsets to section tables.

### Section Headers

Find locations of: - `.text` - `.data` - `.bss` - `.symtab` - relocation
sections

### Symbol Table Entries

Each symbol has: - name\
- value (offset inside section)\
- section index\
- binding (local/global)\
- type (func/object)

### Two Critical Symbol Types

  Type               Meaning
  ------------------ --------------------------------------
  Defined symbol     Function/global in this file
  Undefined symbol   Function/global used but not defined

Undefined symbols are why the linker exists.

------------------------------------------------------------------------

## Step 2 --- Merge Sections (Build the Memory Layout)

Linker must create one big `.text`, one big `.data`, one big `.bss`.

Example:

    main.o  .text size = 100
    foo.o   .text size = 50
    bar.o   .text size = 80

Final layout:

    .text starts at 0x400000
    main.text @ 0x400000
    foo.text  @ 0x400064
    bar.text  @ 0x400096

For each input section you store:

    final_address = base + cumulative_offset

This is called the **linker layout pass**.

------------------------------------------------------------------------

## Step 3 --- Global Symbol Table (The Heart of the Linker)

Gather all symbols from all object files.

Create a map:

    symbol name → final virtual address

### Rules

If symbol is **GLOBAL + DEFINED** → Insert into global table.

If symbol is **GLOBAL + UNDEFINED** → Remember it needs resolution.

If two files define same global symbol\
→ Error: **multiple definition of foo**

Classic linker error.

------------------------------------------------------------------------

## Step 4 --- Symbol Resolution

For every undefined symbol:

    printf
    malloc
    foo

Find it in global symbol table.

If not found → linker error:

    undefined reference to foo

Now you know the final address of every symbol in the program.

But the machine code still contains placeholders.

That's why relocations exist.

------------------------------------------------------------------------

## Step 5 --- Apply Relocations (The Hardest Part)

Relocation entry tells you:

-   offset inside section to patch
-   symbol referenced
-   relocation type
-   addend

Example x86 relocation:

    call printf

Assembler encoded:

    E8 00 00 00 00   ; placeholder

Relocation says:

At offset `0x14` in `.text`:\
Write address of `printf - next_instruction`

Your linker must:

    patch_value = symbol_address + addend - relocation_site
    write 4 bytes into machine code

Different architectures = different relocation formulas.

For a toy linker: Pick **one architecture only** (x86-64 or RISC-V).

Otherwise complexity explodes.

------------------------------------------------------------------------

## Step 6 --- Produce Final ELF Executable (ET_EXEC)

Now you generate a new ELF file.

You must create:

### Program Headers (Segments)

Segments describe memory mapping.

Typical minimal layout:

    LOAD segment 1 → .text (RX)
    LOAD segment 2 → .data + .bss (RW)

### Entry Point

Executable must start at `_start`.

You either: - write your own crt0 - or hardcode entry to `main` (toy
approach)

Set:

    e_entry = address_of(_start or main)

Now write sections + headers → done.

You built a linker.

------------------------------------------------------------------------

## Difficulty Reality Check

  Part                     Difficulty
  ------------------------ ------------
  ELF parsing              Easy
  Section merging          Easy
  Symbol resolution        Medium
  Relocations              Hard
  Writing executable ELF   Medium

The only truly painful part is **relocations**.\
Everything else is bookkeeping.

------------------------------------------------------------------------

## Scope Control (Important)

Do **NOT** try to build GNU ld.

Your first linker should support only:

-   static linking
-   one architecture
-   no shared libraries
-   no PIC
-   no dynamic loader
-   no C++
-   no TLS
-   no weak symbols

Otherwise you will drown.

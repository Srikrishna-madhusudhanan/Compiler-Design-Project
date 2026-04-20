#include <stddef.h>
#include <stdarg.h>

/* Syscall wrappers from io_runtime.s */
extern long __sys_read(int fd, void *buf, size_t count);
extern long __sys_write(int fd, const void *buf, size_t count);
extern void *__sys_brk(void *addr);
extern void __sys_exit(int status);

/* Basic string utilities */
static size_t strlen(const char *s) {
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

static void reverse(char *s, int len) {
    int i = 0, j = len - 1;
    while (i < j) {
        char t = s[i];
        s[i] = s[j];
        s[j] = t;
        i++; j--;
    }
}

static int itoa(long n, char *s, int base) {
    int i = 0;
    int is_negative = 0;
    if (n == 0) { s[i++] = '0'; s[i] = '\0'; return i; }
    if (n < 0 && base == 10) { is_negative = 1; n = -n; }
    while (n != 0) {
        long rem = n % base;
        s[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n = n / base;
    }
    if (is_negative) s[i++] = '-';
    s[i] = '\0';
    reverse(s, i);
    return i;
}

static int utoa(unsigned long n, char *s, int base) {
    int i = 0;
    if (n == 0) { s[i++] = '0'; s[i] = '\0'; return i; }
    while (n != 0) {
        unsigned long rem = n % base;
        s[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n = n / base;
    }
    s[i] = '\0';
    reverse(s, i);
    return i;
}

/* printf implementation */
void printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 'd') {
                int val = va_arg(args, int);
                char buf[32];
                int len = itoa(val, buf, 10);
                __sys_write(1, buf, len);
            } else if (*format == 's') {
                char *s = va_arg(args, char *);
                if (!s) s = "(null)";
                __sys_write(1, s, strlen(s));
            } else if (*format == 'c') {
                char c = (char)va_arg(args, int);
                __sys_write(1, &c, 1);
            } else if (*format == 'x') {
                unsigned int val = va_arg(args, unsigned int);
                char buf[32];
                int len = utoa(val, buf, 16);
                __sys_write(1, buf, len);
            } else if (*format == 'p') {
                void *val = va_arg(args, void *);
                char buf[32];
                __sys_write(1, "0x", 2);
                int len = utoa((unsigned long)val, buf, 16);
                __sys_write(1, buf, len);
            } else if (*format == '%') {
                __sys_write(1, "%", 1);
            }
        } else {
            __sys_write(1, format, 1);
        }
        format++;
    }
    va_end(args);
}

/* scanf implementation (very basic, only %d) */
int scanf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int count = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 'd') {
                int *p = va_arg(args, int *);
                long val = 0;
                char c;
                int sign = 1;
                /* Skip whitespace */
                while (__sys_read(0, &c, 1) == 1 && (c == ' ' || c == '\n' || c == '\t' || c == '\r'));
                if (c == '-') {
                    sign = -1;
                    __sys_read(0, &c, 1);
                }
                while (c >= '0' && c <= '9') {
                    val = val * 10 + (c - '0');
                    if (__sys_read(0, &c, 1) != 1) break;
                }
                *p = (int)(val * sign);
                count++;
            }
        }
        format++;
    }
    va_end(args);
    return count;
}

/* Memory allocator */
static void *heap_current = NULL;

typedef struct Block {
    size_t size;
    int free;
    struct Block *next;
} Block;

static Block *free_list = NULL;

void *malloc(size_t size) {
    if (size == 0) return NULL;
    
    /* Align size to 16 bytes */
    size = (size + 15) & ~15;

    /* Search free list */
    Block *prev = NULL;
    Block *curr = free_list;
    while (curr) {
        if (curr->free && curr->size >= size) {
            curr->free = 0;
            return (void *)(curr + 1);
        }
        prev = curr;
        curr = curr->next;
    }

    /* No block found, expand heap */
    size_t total_size = size + sizeof(Block);
    if (!heap_current) {
        heap_current = __sys_brk(0);
    }
    
    Block *new_block = (Block *)heap_current;
    void *new_heap_end = (void *)((char *)heap_current + total_size);
    if (__sys_brk(new_heap_end) == (void *)-1) return NULL;
    
    new_block->size = size;
    new_block->free = 0;
    new_block->next = NULL;
    
    if (prev) prev->next = new_block;
    else free_list = new_block;
    
    heap_current = new_heap_end;
    return (void *)(new_block + 1);
}

void free(void *ptr) {
    if (!ptr) return;
    Block *block = (Block *)ptr - 1;
    block->free = 1;
}

/* exit wrapper */
void exit(int status) {
    __sys_exit(status);
}

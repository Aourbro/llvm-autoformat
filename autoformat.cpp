/*
 *  Popeye lifts protocol source code in C to its specification in BNF
 *  Copyright (C) 2022 Qingkai Shi <qingkaishi@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <vector>

extern "C" {

// linked list of addresses
struct buf_struct{
    uint8_t *buf_base;      // base addr
    uint64_t buf_len;       // buffer len of this base
    uint64_t buf_offset;    // offset to the original buffer
    uint64_t cur_func;      // current function, when the call stack is poped, every buf_base inside the func should be removed
    buf_struct *next;       // next node
};

static buf_struct* buf_bases_head = 0;  // linked list head
static bool start_logging = 0;          // whether the autoformat begins working
static uint64_t *call_stack = 0;        // call stack
static uint64_t call_stack_len = 0;     // length of the call stack
static uint64_t call_stack_ptr = 0;     // current location of the call stack
FILE *autoformat_fp;                    // file pointer of the log file

/** beginning function, called when enter the parser
 *  initialize the addr list, call stack and open the log file
 */
void autoformat_beginning() {
    assert(start_logging == 0 && "we assume no recursive call on the entry func");
    start_logging = 1;
    buf_struct *buf = (buf_struct *) malloc(sizeof(buf_struct));
    buf->buf_len = -1;
    buf->buf_offset = -1;
    buf->next = NULL;
    buf_bases_head = buf;

    call_stack = (uint64_t *) malloc(1000);
    call_stack_len = 1000;
    call_stack[0] = 0; // ensure at least one element
    call_stack_ptr = 1;

    autoformat_fp = fopen("af_log.txt", "a+");
}

/** end function
 */
void autoformat_end() {
    assert(start_logging);
    assert(call_stack_ptr == 1);
    start_logging = 0;
    free(call_stack);

    fprintf(autoformat_fp, "--------");

    fclose(autoformat_fp);
}

/**
 * @brief add a base addr to the list, 
 * 
 * this will be called after the original buffer address is located, 
 * probably need to be inserted into the source code manually
 * @param base new address that contains any message of the packet
 * @param off offset to the original base address
 * @param len length of the new buffer
 */
void autoformat_add_base(uint8_t *base, uint64_t off, uint64_t len){
    if (!start_logging) return;

    printf("Add base, addr = %p, off = %lu, len = %lu in func %lu\n", base, off, len, call_stack[call_stack_ptr - 1]);
    buf_struct *buf = (buf_struct *) malloc(sizeof(buf_struct));
    buf->buf_base = base;
    buf->buf_len = len;
    buf->buf_offset = off;
    buf->cur_func = call_stack[call_stack_ptr - 1];
    buf->next = buf_bases_head->next;
    buf_bases_head->next = buf;
    return;
}

/**
 * @brief remove bases from the list
 * 
 * @param func the function popping from call stack
 */
void autoformat_remove_base(){
    if (!start_logging) return;

    buf_struct *prev = buf_bases_head;
    buf_struct *crt = buf_bases_head->next;
    uint64_t func = call_stack[call_stack_ptr - 1];
    while(crt){
        if(crt->cur_func == func){
            printf("Remove base, addr = %p in func %lu\n", crt->buf_base, func);
            prev->next = crt->next;
            crt->next = NULL;
            crt = prev->next;
            continue;
        }
        prev = prev->next;
        crt = crt->next;
    }
}

/**
 * @brief print logs
 * 
 * @param ptr base address to be printed
 * @param nbytes n bytes of the buffer to be printed
 */
void autoformat_logging(uint8_t *ptr, uint64_t nbytes) {
    if (!start_logging) return;

    for (unsigned x = 0; x < nbytes; ++x) {
        buf_struct *crt = buf_bases_head->next;
        while(crt){
            uint64_t delta = ((uint64_t) ptr) + x - (uint64_t) crt->buf_base;
            // if the ptr is within one of the buffer's range:
            if (delta < crt->buf_len) {
                // logging the offset and the call stack
                printf("Logging, addr = %p, nbytes = %lu, offset = %lu\n", ptr, nbytes, delta + crt->buf_offset);
                fprintf(autoformat_fp, "[autoformat] %lu, ", delta + crt->buf_offset);
                char c = ptr[x];
                fprintf(autoformat_fp, "%d, ", c);
                for (unsigned y = 0; y < call_stack_ptr; ++y) {
                    fprintf(autoformat_fp, "%lu, ", call_stack[y]);
                }
                fprintf(autoformat_fp, "\n");
            }
            crt = crt->next;
        }
    }
}

/**
 * @brief push a function address to the call stack
 * 
 * @param func_id function address as id
 */
void autoformat_pushing(uint64_t func_id) {
    if (!start_logging) return;
    assert(call_stack);
    if (call_stack_ptr >= call_stack_len) {
        call_stack = (uint64_t *) realloc(call_stack, call_stack_len + 500);
        call_stack_len += 500;
    }
    call_stack[call_stack_ptr++] = func_id;
}

/**
 * @brief pop a function from the call stack
 * 
 */
void autoformat_popping() {
    if (!start_logging) return;
    assert(call_stack_ptr > 0);
    autoformat_remove_base();
    call_stack_ptr--;
}

/**
 * @brief memcpy may use an old buffer and create a new one that needs to be added to the list
 * 
 * @param dest same as memcpy
 * @param src 
 * @param size 
 */
void autoformat_memcpy(uint8_t *dest, uint8_t *src, uint64_t size){
    if (!start_logging) return;
    if(dest == src) return;
    buf_struct *crt = buf_bases_head->next;

    while(crt){
        uint64_t delta = ((uint64_t) src) - (uint64_t) crt->buf_base;
        // if src is within one of the buffer's range
        if (delta < crt->buf_len) {
            buf_struct *crt1 = buf_bases_head->next;
            while(crt1){
                // if dest is one of the buffer's base (pointer reuse)
                if(crt1->buf_base == dest){
                    // update the offset and return
                    crt1->buf_offset = crt->buf_offset + delta;
                    crt1->buf_len = size;
                    printf("Memcpy update, src = %p, dest = %p, size = %lu, off = %lu\n", src, dest, size, crt1->buf_offset);
                    return;
                }
                crt1 = crt1->next;
            }
            /**
             * @brief so dest is a new address for the list
             * 
             * e.g.
             * original buffer is located at 0x0 and the content is "Hello world\00"; 
             * the buf_offset of the original buffer is 0 without any doubt and the buf_len of this buffer is 12 obviously; 
             * 
             * and one of the buf_struct in the list has a buf_base which values 0x100 and the content is "world\00"; 
             * if this buf_struct is added to the list correctly before, it will have a buf_offset which values 6 and a buf_len values 6; 
             * 
             * and now, src values 0x103, dest values 0x200, size values 2
             * which means the program is trying to copy "ld" of "world\00" into a new memory space located at 0x200
             * so the delta will be 3 (0x103 - 0x100), and crt->buf_offset is 6, then the new offset is set to 9 and new len is 2
             * 
             * then if we "load 0x201", the autoformat will print the offset 10 correctly
             */
            printf("Memcpy add, src = %p, dest = %p, size = %lu, off = %lu\n", src, dest, size, delta + crt->buf_offset);
            autoformat_add_base(dest, delta + crt->buf_offset, size);
            return;
        }
        crt = crt->next;
    }
}

/**
 * @brief memcmp may use one or two old buffer(s)
 * 
 * @param mem1 same as memcmp
 * @param mem2 
 * @param size 
 */
void autoformat_memcmp(uint8_t *mem1, uint8_t *mem2, uint64_t size){
    if(!start_logging) return;
    autoformat_logging(mem1, size);
    autoformat_logging(mem2, size);
}

/**
 * @brief memchr may use one old buffer
 * 
 * @param src 
 */
void autoformat_memchr(uint8_t *src){
    if(!start_logging) return;
    if(src) autoformat_logging(src, 1);
}

/* functions below are all used for string operations, so strlen() is free for use */

/**
 * @brief sscanf is similar to memcpy but it's more complex since the varied arguments
 * 
 * @param str original string, probably one of the buffer
 * @param format format that used to read the buffer
 * @param ... many new buffers created with different offset
 */
void autoformat_sscanf(char *str, char *format, ...){
    if (!start_logging) return;
    autoformat_logging((uint8_t *) str, strlen(str));

    va_list arg_ptr;
    va_start(arg_ptr, format);
    int str_len = strlen(str);
    int fmt_len = strlen(format);
    char *buffer = (char *) malloc(fmt_len);
    uint64_t buf_ptr = 0;
    uint64_t str_ptr = 0;
    for(int i = 0; i < fmt_len; ++i, ++str_ptr){
        if(format[i] != '%') buffer[buf_ptr++] = str[i];
        else{
            ++i;
            if(format[i] == '%'){
                buffer[buf_ptr++] = format[i];
            }
            else{
                autoformat_logging((uint8_t *) buffer, buf_ptr);
                memset(buffer, 0, strlen(buffer));
                buf_ptr = 0;
                // so far, we just consider %s in format, %d and others are too complex to handle and is seldom used 
                assert(format[i] == 's');
                char *base = va_arg(arg_ptr, char*);
                autoformat_memcpy((uint8_t *) base, (uint8_t *)(str + str_ptr), strlen(base));
                str_ptr += strlen(base) - 1;
            }
        }
    }
}

/**
 * @brief strcmp, similar to memcmp, but there is no size argument
 * 
 * @param str1 same as strcmp
 * @param str2 
 */
void autoformat_strcmp(char *str1, char *str2){
    if(!start_logging) return;
    autoformat_logging((uint8_t *) str1, strlen(str1));
    autoformat_logging((uint8_t *) str2, strlen(str2));
}

/**
 * @brief strcpy, similar to memcpy, but there is no size argument
 * 
 * @param dest same as strcpy
 * @param src 
 */
void autoformat_strcpy(char *dest, char *src){
    if(!start_logging) return;
    autoformat_logging((uint8_t *) src, strlen(src));
    autoformat_memcpy((uint8_t *) dest, (uint8_t *) src, strlen(src));
}

}

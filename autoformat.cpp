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

struct buf_struct{
    uint8_t *buf_base;
    uint64_t buf_len;
    uint64_t buf_offset;
    buf_struct *next;
};

static buf_struct* buf_bases_head = 0;
static bool start_logging = 0;
static uint64_t *call_stack = 0;
static uint64_t call_stack_len = 0;
static uint64_t call_stack_ptr = 0;
FILE *fp;

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

    fp = fopen("af_log.txt", "a+");
}

void autoformat_end() {
    assert(start_logging);
    assert(call_stack_ptr == 1);
    start_logging = 0;
    free(call_stack);

    fclose(fp);
}

void autoformat_add_base(uint8_t *base, uint64_t off, uint64_t len){
    if (!start_logging) return;

    buf_struct *buf = (buf_struct *) malloc(sizeof(buf_struct));
    buf->buf_base = base;
    buf->buf_len = len;
    buf->buf_offset = off;
    buf->next = buf_bases_head->next;
    buf_bases_head->next = buf;
    return;
}

void autoformat_remove_base(uint8_t *base){
    if (!start_logging) return;

    buf_struct *prev = buf_bases_head;
    buf_struct *crt = buf_bases_head->next;
    while(crt){
        if(crt->buf_base == base){
            prev->next = crt->next;
            crt->next = NULL;
            crt = prev->next;
            continue;
        }
        prev = prev->next;
        crt = crt->next;
    }
}

void autoformat_logging(uint8_t *ptr, uint64_t nbytes) {
    if (!start_logging) return;

    for (unsigned x = 0; x < nbytes; ++x) {
        buf_struct *crt = buf_bases_head->next;
        while(crt){
            uint64_t delta = ((uint64_t) ptr) + x - (uint64_t) crt->buf_base;
            if (delta < crt->buf_len) {
                // logging the offset and the call stack
                fprintf(fp, "[autoformat] %llu, ", delta + crt->buf_offset);
                char c = ptr[x];
                fprintf(fp, "%c, ", c);
                for (unsigned y = 0; y < call_stack_ptr; ++y) {
                    fprintf(fp, "%llu, ", call_stack[y]);
                }
                fprintf(fp, "\n");
            }
            crt = crt->next;
        }
    }
}

void autoformat_pushing(uint64_t func_id) {
    if (!start_logging) return;
    assert(call_stack);
    if (call_stack_ptr >= call_stack_len) {
        call_stack = (uint64_t *) realloc(call_stack, call_stack_len + 500);
        call_stack_len += 500;
    }
    call_stack[call_stack_ptr++] = func_id;
}

void autoformat_popping() {
    if (!start_logging) return;
    assert(call_stack_ptr > 0);
    call_stack_ptr--;
}

void autoformat_memcpy(uint8_t *dest, uint8_t *src, uint64_t size){
    if (!start_logging) return;
    buf_struct *crt = buf_bases_head->next;

    while(crt){
        uint64_t delta = ((uint64_t) src) - (uint64_t) crt->buf_base;
        if (delta < crt->buf_len) {
            buf_struct *crt1 = buf_bases_head->next;
            while(crt1){
                if(crt1->buf_base == dest){
                    crt1->buf_offset = crt->buf_offset + delta;
                    return;
                }
                crt1 = crt1->next;
            }
            autoformat_add_base(dest, delta + crt->buf_offset, size);
            return;
        }
        crt = crt->next;
    }
}

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
                assert(format[i] == 's');
                char *base = va_arg(arg_ptr, char*);
                autoformat_memcpy((uint8_t *) base, (uint8_t *)(str + str_ptr), strlen(base));
                str_ptr += strlen(base) - 1;
            }
        }
    }
}

}

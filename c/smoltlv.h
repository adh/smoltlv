/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*-
 * 
 * SmolTLV - Simple serialization format for JSON/CBOR-like data model 
 * for embedded devices.
 * 
 * Copyright (c) 2025 Aleš Hakl
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef H__SMOLTLV
#define H__SMOLTLV

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMOLTLV_MAX_LENGTH 0xFFFFFFu

typedef enum SmolTLV_Type_e {
    SMOLTLV_TYPE_NULL       = 0x00,
    SMOLTLV_TYPE_BOOL_TRUE  = 0x01,
    SMOLTLV_TYPE_BOOL_FALSE = 0x02,
    SMOLTLV_TYPE_INT        = 0x03,
    SMOLTLV_TYPE_BYTES      = 0x04,
    SMOLTLV_TYPE_STRING     = 0x05,
    SMOLTLV_TYPE_LIST       = 0x06,
    SMOLTLV_TYPE_DICT       = 0x07,
    SMOLTLV_TYPE_MAX     /* = 0x08 */
} SmolTLV_Type;

typedef enum SmolTLV_Status_e {
    SMOLTLV_STATUS_OK = 0,
    SMOLTLV_STATUS_INVALID_ARGUMENT,
    SMOLTLV_STATUS_END,
    SMOLTLV_STATUS_NEED_MORE_DATA,
    SMOLTLV_STATUS_INVALID_FORMAT,
    SMOLTLV_STATUS_INVALID_STATE,
    SMOLTLV_STATUS_OUT_OF_MEMORY,
} SmolTLV_Status;

/*
 * Decoder functionality
 */

typedef struct SmolTLV_Cursor_s {
    const uint8_t *buffer;
    size_t size;
    size_t position;
} SmolTLV_Cursor;

typedef struct SmolTLV_Item_s {
    const uint8_t *pointer;
} SmolTLV_Item;

inline void SmolTLV_Cursor_init(SmolTLV_Cursor *cursor, 
                                const uint8_t *buffer, 
                                size_t size) {
    cursor->buffer = buffer;
    cursor->size = size;
    cursor->position = 0;
}

inline bool SmolTLV_Cursor_is_at_end(SmolTLV_Cursor *cursor) {
    return (cursor->position >= cursor->size);
}

extern SmolTLV_Status SmolTLV_Cursor_next(SmolTLV_Cursor *cursor, 
                                          SmolTLV_Item *item);

extern void SmolTLV_Cursor_for_item(SmolTLV_Cursor *cursor, 
                                    SmolTLV_Item item);

/*
 * Accessors for items
 */


extern SmolTLV_Type SmolTLV_Item_get_type(SmolTLV_Item item);
extern uint8_t SmolTLV_Item_get_type_raw(SmolTLV_Item item);
extern uint32_t SmolTLV_Item_get_length(SmolTLV_Item item);
extern const uint8_t* SmolTLV_Item_get_value(SmolTLV_Item item);

extern bool SmolTLV_Item_is_valid(SmolTLV_Item item);
extern bool SmolTLV_Item_is_null(SmolTLV_Item item);

extern bool SmolTLV_Item_as_bool(SmolTLV_Item item, bool *out);
extern bool SmolTLV_Item_as_int(SmolTLV_Item item, int64_t *out);

extern bool SmolTLV_Item_as_bytes(SmolTLV_Item item, const char **out, 
                                  size_t *out_len);

#ifndef SMOLTLV_NO_MALLOC
/** Accessor for string values - copies to heap, adds \0, which is not 
 * included in out_len */
extern bool SmolTLV_Item_as_string(SmolTLV_Item item, const char **out, 
                                   size_t *out_len);
extern bool SmolTLV_Item_copy_value(SmolTLV_Item item, const char **out, 
                                    size_t *out_len);
#endif

/** String comparison, works for arbitrary items, true if string and 
 * item match */
extern bool SmolTLV_Item_strcmp(SmolTLV_Item item, const char *str);

extern bool SmolTLV_Item_is_container(SmolTLV_Item item);

extern bool SmolTLV_Item_list_at(SmolTLV_Item list_item, 
                                 size_t index, 
                                 SmolTLV_Item *out);
extern bool SmolTLV_Item_dict_get(SmolTLV_Item dict_item, 
                                 const char *key, 
                                 SmolTLV_Item *out);

/*
 * Encoder functionality
 *
 * Note: Encoder does (quite a lot of) heap allocations internally.
 */

#ifndef SMOLTLV_NO_ENCODER
typedef struct SmolTLV_Encoder_s SmolTLV_Encoder;

extern SmolTLV_Encoder* SmolTLV_Encoder_create(void);
extern SmolTLV_Encoder* SmolTLV_Encoder_create_with_size(
    size_t initial_size
);
extern SmolTLV_Encoder* SmolTLV_Encoder_create_from_buffer(
    const uint8_t *buffer,
    size_t size
);
extern void SmolTLV_Encoder_destroy(SmolTLV_Encoder *encoder);
extern SmolTLV_Status SmolTLV_Encoder_finalize(
    SmolTLV_Encoder *encoder,
    const uint8_t **out_buffer,
    size_t *out_size
);

extern SmolTLV_Status SmolTLV_Encoder_write_primitive(SmolTLV_Encoder *encoder, 
                                                      SmolTLV_Type type, 
                                                      const uint8_t *value, 
                                                      size_t length);

extern SmolTLV_Status SmolTLV_Encoder_write_null(SmolTLV_Encoder *encoder);
extern SmolTLV_Status SmolTLV_Encoder_write_bool(SmolTLV_Encoder *encoder, bool value);
extern SmolTLV_Status SmolTLV_Encoder_write_int(SmolTLV_Encoder *encoder, int64_t value);
extern SmolTLV_Status SmolTLV_Encoder_write_bytes(SmolTLV_Encoder *encoder, 
                                                  const uint8_t *data, 
                                                  size_t length);
extern SmolTLV_Status SmolTLV_Encoder_write_string(SmolTLV_Encoder *encoder, 
                                                   const char *str);

extern SmolTLV_Status SmolTLV_Encoder_start_nested(SmolTLV_Encoder *encoder, 
                                                   SmolTLV_Type container_type);
extern SmolTLV_Status SmolTLV_Encoder_start_list(SmolTLV_Encoder *encoder);
extern SmolTLV_Status SmolTLV_Encoder_start_dict(SmolTLV_Encoder *encoder);
extern SmolTLV_Status SmolTLV_Encoder_end(SmolTLV_Encoder *encoder);

#endif /* SMOLTLV_NO_ENCODER */

#ifdef __cplusplus
}
#endif

#endif // H__SMOLTLV
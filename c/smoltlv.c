/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*-
 * 
 * SmolTLV - Simple serialization format for JSON/CBOR-like data model 
 * for embedded devices - C implementation.
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

#include <smoltlv.h>
#include <string.h>
#ifndef SMOLTLV_NO_MALLOC
#include <stdlib.h>
#endif

/*
 * Decoder functionality
 */

static uint32_t load_len24(const uint8_t *p) {
    return ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
}


SmolTLV_Status SmolTLV_Cursor_next(SmolTLV_Cursor *c, SmolTLV_Item *out) {
    if (!c || !out) {
        return SMOLTLV_STATUS_INVALID_ARGUMENT;
    }

    size_t rem = (c->size > c->position) ? (c->size - c->position) : 0;

    if (rem == 0) {
        return SMOLTLV_STATUS_END;
    }

    if (rem < 4) {
        return SMOLTLV_STATUS_NEED_MORE_DATA;
    }

    const uint8_t *p = c->buffer + c->position;
    uint8_t  type = p[0];
    uint32_t len = load_len24(p);

    if (len > 0xFFFFFFu) {
        return SMOLTLV_STATUS_INVALID_FORMAT;
    }

    if (type == SMOLTLV_TYPE_NULL && len != 0u) {
        return SMOLTLV_STATUS_INVALID_FORMAT;
    }

    if (type == SMOLTLV_TYPE_BOOL_TRUE && len != 0u) {
        return SMOLTLV_STATUS_INVALID_FORMAT;
    }

    if (type == SMOLTLV_TYPE_BOOL_FALSE && len != 0u) {
        return SMOLTLV_STATUS_INVALID_FORMAT;
    }

    if (type == SMOLTLV_TYPE_INT && len != 8u) {
        return SMOLTLV_STATUS_INVALID_FORMAT;
    }

    if (rem < 4u + (size_t)len) {
        return SMOLTLV_STATUS_NEED_MORE_DATA;
    }

    out->pointer = p;
    c->position += 4u + (size_t)len;
    return SMOLTLV_STATUS_OK;
}

void SmolTLV_Cursor_for_item(SmolTLV_Cursor *c, SmolTLV_Item item) {
    // XXX: this works even for primitive values, which maybe is not ideal
    if (!c || !item.pointer) {
        return;
    }

    const uint8_t *p = item.pointer;
    uint32_t len = load_len24(p);
    c->buffer = p + 4;
    c->size = len;
    c->position = 0;
}

/*
 * Accessors for items
 */

SmolTLV_Type SmolTLV_Item_get_type(SmolTLV_Item item) {
    return (SmolTLV_Type)(item.pointer[0]);
}

uint8_t SmolTLV_Item_get_type_raw(SmolTLV_Item item) {
    return item.pointer[0];
}

uint32_t SmolTLV_Item_get_length(SmolTLV_Item item) {
    return load_len24(item.pointer);
}

const uint8_t* SmolTLV_Item_get_value(SmolTLV_Item item) {
    return item.pointer + 4;
}

bool SmolTLV_Item_is_valid(SmolTLV_Item item) {
    if (item.pointer == NULL) {
        return false;
    }
    if (SmolTLV_Item_get_type(item) >= SMOLTLV_TYPE_MAX) {
        return false;
    }
    return true;
}

bool SmolTLV_Item_is_null(SmolTLV_Item item) {
    return SmolTLV_Item_get_type(item) == SMOLTLV_TYPE_NULL;
}

bool SmolTLV_Item_as_bool(SmolTLV_Item item, bool *out) {
    SmolTLV_Type type = SmolTLV_Item_get_type(item);
    if (type == SMOLTLV_TYPE_BOOL_TRUE) {
        if (out) *out = true;
        return true;
    } else if (type == SMOLTLV_TYPE_BOOL_FALSE) {
        if (out) *out = false;
        return true;
    }
    return false;
}

bool SmolTLV_Item_as_int(SmolTLV_Item item, int64_t *out) {
    SmolTLV_Type type = SmolTLV_Item_get_type(item);
    if (type != SMOLTLV_TYPE_INT) {
        return false;
    }

    if (SmolTLV_Item_get_length(item) != 8u) {
        return false;
    }

    const uint8_t *p = SmolTLV_Item_get_value(item);
    int64_t value = ((int64_t)p[0] << 56) |
                    ((int64_t)p[1] << 48) |
                    ((int64_t)p[2] << 40) |
                    ((int64_t)p[3] << 32) |
                    ((int64_t)p[4] << 24) |
                    ((int64_t)p[5] << 16) |
                    ((int64_t)p[6] << 8)  |
                    ((int64_t)p[7]);
    if (out) {
        *out = value;
    }
    return true;
}

bool SmolTLV_Item_as_bytes(SmolTLV_Item item, const char **out, size_t *out_len) {
    SmolTLV_Type type = SmolTLV_Item_get_type(item);
    if (type != SMOLTLV_TYPE_BYTES) {
        return false;
    }

    if (out) {
        *out = (const char *)SmolTLV_Item_get_value(item);
    }
    if (out_len) {
        *out_len = SmolTLV_Item_get_length(item);
    }
    return true;
}
#ifndef SMOLTLV_NO_MALLOC
bool SmolTLV_Item_as_string(SmolTLV_Item item, const char **out, size_t *out_len) {
    SmolTLV_Type type = SmolTLV_Item_get_type(item);
    if (type != SMOLTLV_TYPE_STRING)
        return false;

    size_t len = SmolTLV_Item_get_length(item);
    if (out) {
        uint8_t *buf = (uint8_t *)malloc(len + 1u);
        if (!buf)
            return false;

        memcpy(buf, SmolTLV_Item_get_value(item), len);
        buf[len] = '\0';
        *out = (const char *)buf;
    }

    if (out_len) {
        *out_len = len;
    }

    return true;
}

bool SmolTLV_Item_copy_value(SmolTLV_Item item, const char **out, size_t *out_len) {
    size_t len = SmolTLV_Item_get_length(item);
    if (out) {
        uint8_t *buf = (uint8_t *)malloc(len);
        if (!buf)
            return false;

        memcpy(buf, SmolTLV_Item_get_value(item), len);
        *out = (const char *)buf;
    }

    if (out_len) {
        *out_len = len;
    }

    return true;
}
#endif

bool SmolTLV_Item_strcmp(SmolTLV_Item item, const char *str) {
    if (!str) {
        return false;
    }

    SmolTLV_Type type = SmolTLV_Item_get_type(item);
    if (type != SMOLTLV_TYPE_STRING) {
        return false;
    }

    size_t item_len = SmolTLV_Item_get_length(item);
    size_t str_len = strlen(str);

    if (item_len != str_len) {
        return false;
    }

    const uint8_t *item_value = SmolTLV_Item_get_value(item);
    if (memcmp(item_value, str, item_len) != 0) {
        return false;
    }

    return true;
}

bool SmolTLV_Item_is_container(SmolTLV_Item item) {
    SmolTLV_Type type = SmolTLV_Item_get_type(item);
    return (type == SMOLTLV_TYPE_LIST || type == SMOLTLV_TYPE_DICT);
}

bool SmolTLV_Item_list_at(SmolTLV_Item list_item,
                          size_t index,
                          SmolTLV_Item *out_item) {
    if (SmolTLV_Item_get_type(list_item) != SMOLTLV_TYPE_LIST) {
        return false;
    }

    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_for_item(&cursor, list_item);

    size_t current_index = 0;
    SmolTLV_Status status;
    SmolTLV_Item item;

    while ((status = SmolTLV_Cursor_next(&cursor, &item)) == SMOLTLV_STATUS_OK) {
        if (current_index == index) {
            if (out_item) {
                *out_item = item;
            }
            return true;
        }
        current_index++;
    }

    return false;
}

bool SmolTLV_Item_dict_get(SmolTLV_Item dict_item,
                           const char *key,
                           SmolTLV_Item *out_item) {
    if (SmolTLV_Item_get_type(dict_item) != SMOLTLV_TYPE_DICT) {
        return false;
    }

    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_for_item(&cursor, dict_item);

    SmolTLV_Status status;
    SmolTLV_Item key_item;
    SmolTLV_Item value_item;

    while ((status = SmolTLV_Cursor_next(&cursor, &key_item)) == SMOLTLV_STATUS_OK) {
        if (SmolTLV_Item_get_type(key_item) != SMOLTLV_TYPE_STRING) {
            return false;
        }

        if (!SmolTLV_Item_strcmp(key_item, key)) {
            // Key does not match, skip value
            status = SmolTLV_Cursor_next(&cursor, &value_item);
            if (status != SMOLTLV_STATUS_OK) {
                return false;
            }
            continue;
        }

        // Key matches, get value
        status = SmolTLV_Cursor_next(&cursor, &value_item);
        if (status != SMOLTLV_STATUS_OK) {
            return false;
        }

        if (out_item) {
            *out_item = value_item;
        }
        return true;
    }

    return false;
}

/*
 * Encoder functionality
 */

#ifndef SMOLTLV_NO_ENCODER

typedef struct EncoderFrame_s EncoderFrame;
struct EncoderFrame_s {
    size_t start_position;
    EncoderFrame *next;
};

struct SmolTLV_Encoder_s {
    uint8_t *buffer;
    size_t buffer_size;
    size_t position;
    bool error: 1;
    bool finalized: 1;
    bool manage_buffer: 1;
    EncoderFrame *frame_stack;
};

SmolTLV_Encoder* SmolTLV_Encoder_create() {
    return SmolTLV_Encoder_create_with_size(4u);
}

SmolTLV_Encoder* SmolTLV_Encoder_create_with_size(size_t initial_size) {
    SmolTLV_Encoder *encoder = (SmolTLV_Encoder *)malloc(sizeof(SmolTLV_Encoder));
    if (!encoder) {
        return NULL;
    }

    encoder->buffer = (uint8_t *)malloc(initial_size);
    if (!encoder->buffer) {
        free(encoder);
        return NULL;
    }

    encoder->buffer_size = initial_size;
    encoder->position = 0;
    encoder->error = false;
    encoder->finalized = false;
    encoder->manage_buffer = true;
    encoder->frame_stack = NULL;
    return encoder;
}

SmolTLV_Encoder* SmolTLV_Encoder_create_from_buffer(
    const uint8_t *buffer,
    size_t size
) {
    SmolTLV_Encoder *encoder = (SmolTLV_Encoder *)malloc(sizeof(SmolTLV_Encoder));
    if (!encoder) {
        return NULL;
    }

    encoder->buffer = (uint8_t *)buffer;
    encoder->buffer_size = size;
    encoder->position = 0;
    encoder->error = false;
    encoder->finalized = false;
    encoder->manage_buffer = false;
    encoder->frame_stack = NULL;
    return encoder;
}

void SmolTLV_Encoder_destroy(SmolTLV_Encoder *encoder) {
    if (!encoder) {
        return;
    }

    if (encoder->manage_buffer && encoder->buffer) {
        free(encoder->buffer);
    }

    EncoderFrame *frame = encoder->frame_stack;
    while (frame) {
        EncoderFrame *next = frame->next;
        free(frame);
        frame = next;
    }

    free(encoder);
}

SmolTLV_Status SmolTLV_Encoder_finalize(
    SmolTLV_Encoder *encoder,
    const uint8_t **out_buffer,
    size_t *out_size
) {
    if (!encoder){
        return SMOLTLV_STATUS_INVALID_ARGUMENT;
    }

    if (encoder->manage_buffer && (!out_buffer || !out_size)) {
        return SMOLTLV_STATUS_INVALID_ARGUMENT;
    }

    if (encoder->error) {
        return SMOLTLV_STATUS_INVALID_STATE;
    }

    if (encoder->frame_stack != NULL) {
        return SMOLTLV_STATUS_INVALID_STATE;
    }

    encoder->finalized = true;
    encoder->manage_buffer = false;

    *out_buffer = encoder->buffer;
    *out_size = encoder->position;

    return SMOLTLV_STATUS_OK;
}


static bool encoder_reserve(SmolTLV_Encoder *encoder, size_t size) {
    if (encoder->position + size <= encoder->buffer_size) {
        return true;
    }

    if (!encoder->manage_buffer) {
        encoder->error = true;
        return false;
    }

    size_t new_size = encoder->buffer_size * 2u;
    while (new_size < encoder->position + size) {
        new_size *= 2u;
    }

    uint8_t *new_buffer = (uint8_t *)realloc(encoder->buffer, new_size);
    if (!new_buffer) {
        encoder->error = true;
        return false;
    }

    encoder->buffer = new_buffer;
    encoder->buffer_size = new_size;
    return true;
}

bool encoder_write_header(SmolTLV_Encoder *encoder, 
                          SmolTLV_Type type, 
                          uint32_t length) {
    if (encoder->error || encoder->finalized) {
        return false;
    }

    if (!encoder_reserve(encoder, 4u)) {
        return false;
    }

    encoder->buffer[encoder->position + 0u] = (uint8_t)type;
    encoder->buffer[encoder->position + 1u] = (uint8_t)((length >> 16) & 0xFFu);
    encoder->buffer[encoder->position + 2u] = (uint8_t)((length >> 8) & 0xFFu);
    encoder->buffer[encoder->position + 3u] = (uint8_t)(length & 0xFFu);
    encoder->position += 4u;
    return true;
}
void encoder_patch_header(SmolTLV_Encoder *encoder, 
                          uint32_t length, 
                          size_t header_position) {
    if (encoder->error || encoder->finalized) {
        return;
    }

    encoder->buffer[header_position + 1u] = (uint8_t)((length >> 16) & 0xFFu);
    encoder->buffer[header_position + 2u] = (uint8_t)((length >> 8) & 0xFFu);
    encoder->buffer[header_position + 3u] = (uint8_t)(length & 0xFFu);
}

SmolTLV_Status SmolTLV_Encoder_write_primitive(SmolTLV_Encoder *encoder, 
                                               SmolTLV_Type type, 
                                               const uint8_t *value, 
                                               size_t length) {
    if (encoder->error || encoder->finalized) {
        return SMOLTLV_STATUS_INVALID_STATE;
    }

    if (length > SMOLTLV_MAX_LENGTH) {
        return SMOLTLV_STATUS_INVALID_ARGUMENT;
    }

    // Reserve space
    if (!encoder_reserve(encoder, 4u + (size_t)length)) {
        return SMOLTLV_STATUS_OUT_OF_MEMORY;
    }

    // Write header
    if (!encoder_write_header(encoder, type, length)) {
        return SMOLTLV_STATUS_OUT_OF_MEMORY;
    }

    // Write value
    if (length > 0u && value != NULL) {
        memcpy(&encoder->buffer[encoder->position], value, (size_t)length);
        encoder->position += (size_t)length;
    }

    return SMOLTLV_STATUS_OK;
}

SmolTLV_Status SmolTLV_Encoder_write_null(SmolTLV_Encoder *encoder) {
    return SmolTLV_Encoder_write_primitive(encoder, SMOLTLV_TYPE_NULL, NULL, 0u);
}
SmolTLV_Status SmolTLV_Encoder_write_bool(SmolTLV_Encoder *encoder, bool value) {
    SmolTLV_Type type = value ? SMOLTLV_TYPE_BOOL_TRUE : SMOLTLV_TYPE_BOOL_FALSE;
    return SmolTLV_Encoder_write_primitive(encoder, type, NULL, 0u);
}
SmolTLV_Status SmolTLV_Encoder_write_int(SmolTLV_Encoder *encoder, int64_t value) {
    uint8_t buf[8];
    buf[0] = (uint8_t)((value >> 56) & 0xFFu);
    buf[1] = (uint8_t)((value >> 48) & 0xFFu);
    buf[2] = (uint8_t)((value >> 40) & 0xFFu);
    buf[3] = (uint8_t)((value >> 32) & 0xFFu);
    buf[4] = (uint8_t)((value >> 24) & 0xFFu);
    buf[5] = (uint8_t)((value >> 16) & 0xFFu);
    buf[6] = (uint8_t)((value >> 8) & 0xFFu);
    buf[7] = (uint8_t)(value & 0xFFu);
    return SmolTLV_Encoder_write_primitive(encoder, SMOLTLV_TYPE_INT, buf, 8u);
}
SmolTLV_Status SmolTLV_Encoder_write_bytes(SmolTLV_Encoder *encoder, 
                                           const uint8_t *data, 
                                           size_t length) {
    return SmolTLV_Encoder_write_primitive(encoder, SMOLTLV_TYPE_BYTES, data, length);
}
SmolTLV_Status SmolTLV_Encoder_write_string(SmolTLV_Encoder *encoder, 
                                            const char *str) {
    if (!str) {
        return SMOLTLV_STATUS_INVALID_ARGUMENT;
    }

    size_t length = strlen(str);
    return SmolTLV_Encoder_write_primitive(encoder, SMOLTLV_TYPE_STRING, (const uint8_t *)str, length);
}

SmolTLV_Status SmolTLV_Encoder_start_nested(SmolTLV_Encoder *encoder, 
                                            SmolTLV_Type container_type) {
    if (encoder->error) {
        return SMOLTLV_STATUS_INVALID_STATE;
    }

    if (container_type != SMOLTLV_TYPE_LIST && container_type != SMOLTLV_TYPE_DICT) {
        return SMOLTLV_STATUS_INVALID_ARGUMENT;
    }

    // Reserve space for header
    if (!encoder_reserve(encoder, 4u)) {
        return SMOLTLV_STATUS_OUT_OF_MEMORY;
    }

    // Push frame onto stack
    EncoderFrame *frame = (EncoderFrame *)malloc(sizeof(EncoderFrame));
    if (!frame) {
        return SMOLTLV_STATUS_OUT_OF_MEMORY;
    }
    frame->start_position = encoder->position;
    frame->next = encoder->frame_stack;
    encoder->frame_stack = frame;

    // Write placeholder header
    if (!encoder_write_header(encoder, container_type, 0u)) {
        return SMOLTLV_STATUS_OUT_OF_MEMORY;
    }

    return SMOLTLV_STATUS_OK;
}
SmolTLV_Status SmolTLV_Encoder_start_list(SmolTLV_Encoder *encoder) {
    return SmolTLV_Encoder_start_nested(encoder, SMOLTLV_TYPE_LIST);
}
SmolTLV_Status SmolTLV_Encoder_start_dict(SmolTLV_Encoder *encoder) {
    return SmolTLV_Encoder_start_nested(encoder, SMOLTLV_TYPE_DICT);
}
SmolTLV_Status SmolTLV_Encoder_end(SmolTLV_Encoder *encoder) {
    if (encoder->error || encoder->finalized) {
        return SMOLTLV_STATUS_INVALID_STATE;
    }
    if (!encoder->frame_stack) {
        return SMOLTLV_STATUS_INVALID_STATE;
    }
    // Pop frame from stack
    EncoderFrame *frame = encoder->frame_stack;
    encoder->frame_stack = frame->next;

    // Calculate length of nested container
    size_t container_start = frame->start_position + 4u;
    size_t container_length = encoder->position - container_start;

    if (container_length > SMOLTLV_MAX_LENGTH) {
        free(frame);
        encoder->error = true;
        return SMOLTLV_STATUS_INVALID_FORMAT;
    }


    // Patch header with correct length
    encoder_patch_header(encoder, (uint32_t)container_length, frame->start_position);

    free(frame);

    return SMOLTLV_STATUS_OK;
}

#endif /* SMOLTLV_NO_ENCODER */
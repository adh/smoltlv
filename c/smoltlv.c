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
    c->buffer = p;
    c->size = 4u + (size_t)len;
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

struct SmolTLV_Encoder_s {
    uint8_t *buffer;
    size_t buffer_size;
    size_t position;
};


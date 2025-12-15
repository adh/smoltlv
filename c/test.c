#include <smoltlv.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

uint8_t test_null[] = {
    0x00, 0x00, 0x00, 0x00
};

void test_decode_null() {
    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_init(&cursor, test_null, sizeof(test_null));

    SmolTLV_Item item;
    SmolTLV_Status status = SmolTLV_Cursor_next(&cursor, &item);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to decode item: %d\n", status);
        return;
    }

    if (!SmolTLV_Item_is_null(item)) {
        printf("Decoded item is not null\n");
        return;
    }

    printf("Successfully decoded null item\n");
}

uint8_t test_bool_true[] = {
    0x01, 0x00, 0x00, 0x00
};

uint8_t test_bool_false[] = {
    0x02, 0x00, 0x00, 0x00
};

void test_decode_bool() {
    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_init(&cursor, test_bool_true, sizeof(test_bool_true));

    SmolTLV_Item item;
    SmolTLV_Status status = SmolTLV_Cursor_next(&cursor, &item);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to decode item: %d\n", status);
        return;
    }

    bool value;
    if (!SmolTLV_Item_as_bool(item, &value)) {
        printf("Decoded item is not a boolean\n");
        return;
    }

    if (value) {
        printf("Successfully decoded boolean true\n");
    } else {
        printf("Decoded boolean is false, expected true\n");
    }

    SmolTLV_Cursor_init(&cursor, test_bool_false, sizeof(test_bool_false));
    status = SmolTLV_Cursor_next(&cursor, &item);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to decode item: %d\n", status);
        return;
    }

    if (!SmolTLV_Item_as_bool(item, &value)) {
        printf("Decoded item is not a boolean\n");
        return;
    }

    if (!value) {
        printf("Successfully decoded boolean false\n");
    } else {
        printf("Decoded boolean is true, expected false\n");
    }
}

uint8_t test_integer[] = {
    0x03, 0x00, 0x00, 0x08, // Type: INT, Length: 8
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A  // Value: 42
};

void test_decode_integer() {
    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_init(&cursor, test_integer, sizeof(test_integer));

    SmolTLV_Item item;
    SmolTLV_Status status = SmolTLV_Cursor_next(&cursor, &item);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to decode item: %d\n", status);
        return;
    }

    int64_t value;
    if (!SmolTLV_Item_as_int(item, &value)) {
        printf("Decoded item is not an integer\n");
        return;
    }

    if (value == 42) {
        printf("Successfully decoded integer 42\n");
    } else {
        printf("Decoded integer is %lld, expected 42\n", value);
    }
}

uint8_t test_negative_integer[] = {
    0x03, 0x00, 0x00, 0x08, // Type: INT, Length: 8
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xD6  // Value: -42
};

void test_decode_negative_integer() {
    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_init(&cursor, test_negative_integer, sizeof(test_negative_integer));

    SmolTLV_Item item;
    SmolTLV_Status status = SmolTLV_Cursor_next(&cursor, &item);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to decode item: %d\n", status);
        return;
    }

    int64_t value;
    if (!SmolTLV_Item_as_int(item, &value)) {
        printf("Decoded item is not an integer\n");
        return;
    }

    if (value == -42) {
        printf("Successfully decoded integer -42\n");
    } else {
        printf("Decoded integer is %lld, expected -42\n", value);
    }
}

uint8_t test_list[] = {
    0x06, 0x00, 0x00, 0x14, // Type: LIST, Length: 20
    0x01, 0x00, 0x00, 0x00, // BOOL TRUE
    0x02, 0x00, 0x00, 0x00, // BOOL FALSE
    0x03, 0x00, 0x00, 0x08, // INT
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x2A  // Value: 42
};

void test_decode_list() {
    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_init(&cursor, test_list, sizeof(test_list));

    SmolTLV_Item list_item;
    SmolTLV_Status status = SmolTLV_Cursor_next(&cursor, &list_item);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to decode list item: %d\n", status);
        return;
    }

    if (SmolTLV_Item_get_type(list_item) != SMOLTLV_TYPE_LIST) {
        printf("Decoded item is not a list\n");
        return;
    }

    SmolTLV_Item item;
    if (!SmolTLV_Item_list_at(list_item, 0, &item)) {
        printf("Failed to get first item from list\n");
        return;
    }

    bool bool_value;
    if (!SmolTLV_Item_as_bool(item, &bool_value) || !bool_value) {
        printf("First item in list is not BOOL TRUE\n");
        return;
    }

    if (!SmolTLV_Item_list_at(list_item, 1, &item)) {
        printf("Failed to get second item from list\n");
        return;
    }

    if (!SmolTLV_Item_as_bool(item, &bool_value) || bool_value) {
        printf("Second item in list is not BOOL FALSE\n");
        return;
    }

    if (!SmolTLV_Item_list_at(list_item, 2, &item)) {
        printf("Failed to get third item from list\n");
        return;
    }

    int64_t int_value;
    if (!SmolTLV_Item_as_int(item, &int_value) || int_value != 42) {
        printf("Third item in list is not INT 42\n");
        return;
    }

    printf("Successfully decoded list with BOOL TRUE, BOOL FALSE, INT 42\n");
}

uint8_t test_dict[] = {
    0x07, 0x00, 0x00, 0x24, // Type: DICT, Length: 36
    // Key: "age"
    0x05, 0x00, 0x00, 0x03, // STRING
    'a', 'g', 'e',
    // Value: 30
    0x03, 0x00, 0x00, 0x08, // INT
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1E,
    // Key: "name"
    0x05, 0x00, 0x00, 0x04, // STRING
    'n', 'a', 'm', 'e',
    // Value: "Alice"
    0x05, 0x00, 0x00, 0x05, // STRING
    'A', 'l', 'i', 'c', 'e'
};

void test_decode_dict() {
    SmolTLV_Cursor cursor;
    SmolTLV_Cursor_init(&cursor, test_dict, sizeof(test_dict));

    SmolTLV_Item dict_item;
    SmolTLV_Status status = SmolTLV_Cursor_next(&cursor, &dict_item);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to decode dict item: %d\n", status);
        return;
    }

    if (SmolTLV_Item_get_type(dict_item) != SMOLTLV_TYPE_DICT) {
        printf("Decoded item is not a dict\n");
        return;
    }

    SmolTLV_Item item;
    if (!SmolTLV_Item_dict_get(dict_item, "age", &item)) {
        printf("Failed to get 'age' from dict\n");
        return;
    }

    int64_t age;
    if (!SmolTLV_Item_as_int(item, &age) || age != 30) {
        printf("'age' is not INT 30\n");
        return;
    }

    if (!SmolTLV_Item_dict_get(dict_item, "name", &item)) {
        printf("Failed to get 'name' from dict\n");
        return;
    }

    const uint8_t *name_value = SmolTLV_Item_get_value(item);
    uint32_t name_length = SmolTLV_Item_get_length(item);
    char name[ name_length + 1];
    memcpy(name, name_value, name_length);
    name[name_length] = '\0';

    if (strcmp(name, "Alice") != 0) {
        printf("'name' is not 'Alice'\n");
        return;
    }

    printf("Successfully decoded dict with age=30 and name='Alice'\n");
}

void test_encode_null() {
    SmolTLV_Encoder *encoder = SmolTLV_Encoder_create();
    if (!encoder) {
        printf("Failed to create encoder\n");
        return;
    }

    SmolTLV_Status status = SmolTLV_Encoder_write_null(encoder);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to encode null: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    const uint8_t *buffer;
    size_t size;
    status = SmolTLV_Encoder_finalize(encoder, &buffer, &size);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to finalize encoding: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    printf("Successfully encoded null, size: %zu\n", size);

    if (size != 4 || buffer[0] != 0x00 || buffer[1] != 0x00 || buffer[2] != 0x00 || buffer[3] != 0x00) {
        printf("Encoded null does not match expected output\n");
    } else {
        printf("Encoded null matches expected output\n");
    }
    free((void*)buffer);
    SmolTLV_Encoder_destroy(encoder);
}

void test_encode_int() {
    SmolTLV_Encoder *encoder = SmolTLV_Encoder_create();
    if (!encoder) {
        printf("Failed to create encoder\n");
        return;
    }

    SmolTLV_Status status = SmolTLV_Encoder_write_int(encoder, -42);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to encode int: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    const uint8_t *buffer;
    size_t size;
    status = SmolTLV_Encoder_finalize(encoder, &buffer, &size);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to finalize encoding: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    printf("Successfully encoded int, size: %zu\n", size);

    if (memcmp(buffer, test_negative_integer, size) != 0) {
        printf("Encoded int does not match expected output\n");
    } else {
        printf("Encoded int matches expected output\n");
    }

    free((void*)buffer);
    SmolTLV_Encoder_destroy(encoder);
}

void test_encode_dict(){
    SmolTLV_Encoder *encoder = SmolTLV_Encoder_create();
    if (!encoder) {
        printf("Failed to create encoder\n");
        return;
    }

    SmolTLV_Status status = SmolTLV_Encoder_start_dict(encoder);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to start dict: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    // Key: "age"
    status = SmolTLV_Encoder_write_string(encoder, "age");
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to write key 'age': %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    // Value: 30
    status = SmolTLV_Encoder_write_int(encoder, 30);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to write value 30: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    // Key: "name"
    status = SmolTLV_Encoder_write_string(encoder, "name");
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to write key 'name': %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    // Value: "Alice"
    status = SmolTLV_Encoder_write_string(encoder, "Alice");
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to write value 'Alice': %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    status = SmolTLV_Encoder_end(encoder);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to end dict: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    const uint8_t *buffer;
    size_t size;
    status = SmolTLV_Encoder_finalize(encoder, &buffer, &size);
    if (status != SMOLTLV_STATUS_OK) {
        printf("Failed to finalize encoding: %d\n", status);
        SmolTLV_Encoder_destroy(encoder);
        return;
    }

    printf("Successfully encoded dict, size: %zu\n", size);

    if (memcmp(buffer, test_dict, size) != 0) {
        printf("Encoded dict does not match expected output\n");
    } else {
        printf("Encoded dict matches expected output\n");
    }

    free((void*)buffer);
    SmolTLV_Encoder_destroy(encoder);
}

int main() {
    test_decode_null();
    test_decode_bool();
    test_decode_integer();
    test_decode_negative_integer();
    test_decode_list();
    test_decode_dict();

    test_encode_null();
    test_encode_int();
    test_encode_dict();
    return 0;
}
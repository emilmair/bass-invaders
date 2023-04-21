#ifndef MATHS_H
#define MATHS_H

#define dd decimal_digit
#define pot power_of_ten

#include <stdint.h>

/// returns ten to the power of n
static uint32_t power_of_ten(uint32_t n) {
    if (n == 0) return 1;
    uint32_t r = 10;
    for (uint32_t i = 0; i < n-1; i++) {r *= 10;}
    return r;
}

/// returns decimal digit at the nth position in given number
static uint8_t decimal_digit(uint32_t num, uint32_t n) {
    uint32_t r;
    r = num / power_of_ten(n);
    r = r % 10;
    return (uint8_t) r;
}

static uint32_t absolute(int32_t num) {
    return (num<0)?-num:num;
}

static uint32_t difference(uint32_t num1, uint32_t num2) {
    return absolute((int32_t) num1 - (int32_t) num2);
}

struct List {
    uint8_t* data;
    uint32_t size;
};

typedef struct List List;

static List list_create(uint32_t size) {
    return (List) {(uint8_t*) malloc(size), size};
}

static void list_set(List* list, uint32_t index, uint8_t value) {
    if (index < list->size) return;
    list->data[index] = value;
}

static uint8_t list_get(List* list, uint32_t index) {
    if (index > list->size) return 0;
    return list->data[index];
}

static void list_append(List* list, uint8_t value) {
    list->size++;
    list->data = realloc(list->data, list->size);
    list->data[list->size-1] = value;
}

static void list_remove(List* list, uint32_t index) {
    if (index > list->size) return;
    list->size--;
    memmove(list->data+index, list->data+index+1, list->size);
    list->data = realloc(list->data, list->size);
}

static uint8_t list_pop(List* list, uint32_t index) {
    if (index > list->size) return 0;
    uint8_t value = list_get(list, index);
    list_remove(list, index);
    return value;
}

#endif //MATHS_H

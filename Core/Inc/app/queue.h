#pragma once
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#define QUEUE_DECLARE(NAME, TYPE, CAPACITY) \
	typedef struct NAME { \
		uint16_t head; \
		uint16_t tail; \
		uint16_t size; \
		TYPE buffer[CAPACITY]; \
	} NAME ; \
			\
	static inline void NAME##_Init(NAME* q) { \
		q->head = q->tail = q->size = 0; \
	} \
		\
	static inline uint16_t NAME##_GetCapacity(const NAME* q) { \
		return CAPACITY; \
	} \
		\
	static inline uint16_t NAME##_GetSize(const NAME* q) { \
		return q->size; \
	} \
		\
	static inline bool NAME##_IsEmpty(const NAME* q) { \
		return q->size == 0; \
	} \
		\
	static inline bool NAME##_IsFull(const NAME* q) { \
		return q->size == (CAPACITY); \
	} \
		\
	static inline int16_t NAME##_Push(NAME* q, TYPE value) { \
		if (NAME##_IsFull(q)) return -1; \
		q->buffer[q->tail] = value; \
		q->tail = (q->tail + 1) % CAPACITY; \
		q->size += 1; \
		return 0; \
	} \
		\
	static inline int16_t NAME##_Pop(NAME* q, TYPE* out) { \
		if (NAME##_IsEmpty(q)) return -1; \
		if (out) *out = q->buffer[q->head]; \
		q->head = (q->head + 1) % CAPACITY; \
		q->size -= 1; \
		return 0; \
	} \
		\
	static inline int16_t NAME##_Front(const NAME* q, TYPE* out) { \
		if (NAME##_IsEmpty(q)) return -1; \
		if (out) *out = q->buffer[q->head]; \
		return 0; \
	} \
		\
	static inline void NAME##_Clear(NAME* q) { \
		q->head = q->tail = q->size = 0; \
		memset(q->buffer, 0, CAPACITY * sizeof(TYPE)); \
	} \
		\
	static inline uint16_t NAME##_At(NAME* q, uint16_t idx, TYPE* out) { \
		if (idx >= q->size) return -1; \
		if (out) *out = q->buffer[q->head + idx % CAPACITY]; \
		return 0; \
	} \

/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#ifndef COMMON_BYTEORDER_H_
#define COMMON_BYTEORDER_H_

#include <stdint.h>

namespace common {

inline void Set8(void* buff, const uint32_t offset, const uint8_t value) {
	static_cast<uint8_t*>(buff)[offset] = value;
}

inline void SetBE16(void* buff, const uint16_t value) {
	Set8(buff, 0, static_cast<uint8_t>(value >> 8));
	Set8(buff, 1, static_cast<uint8_t>(value));
}

inline void SetBE32(void* buff, const uint32_t value) {
	Set8(buff, 0, static_cast<uint8_t>(value >> 24));
	Set8(buff, 1, static_cast<uint8_t>(value >> 16));
	Set8(buff, 2, static_cast<uint8_t>(value >> 8));
	Set8(buff, 3, static_cast<uint8_t>(value));
}

inline void SetBE64(void* buff, const uint64_t value) {
	Set8(buff, 0, static_cast<uint8_t>(value >> 56));
	Set8(buff, 1, static_cast<uint8_t>(value >> 48));
	Set8(buff, 2, static_cast<uint8_t>(value >> 40));
	Set8(buff, 3, static_cast<uint8_t>(value >> 32));
	Set8(buff, 4, static_cast<uint8_t>(value >> 24));
	Set8(buff, 5, static_cast<uint8_t>(value >> 16));
	Set8(buff, 6, static_cast<uint8_t>(value >> 8));
	Set8(buff, 7, static_cast<uint8_t>(value));
}

inline uint8_t Get8(const void* buff, const uint32_t offset) {
	return static_cast<const uint8_t*>(buff)[offset];
}

inline uint16_t GetBE16(const void* buff) {
	return static_cast<uint16_t>((Get8(buff, 0) << 8) | (Get8(buff, 1) << 0));
}

inline uint32_t GetBE32(const void* buff) {
	return (static_cast<uint32_t>(Get8(buff, 0)) << 24) |
		   (static_cast<uint32_t>(Get8(buff, 1)) << 16) |
		   (static_cast<uint32_t>(Get8(buff, 2)) << 8) |
		   (static_cast<uint32_t>(Get8(buff, 3)));
}

inline uint64_t GetBE64(const void* buff) {
	return (static_cast<uint64_t>(Get8(buff, 0)) << 56) |
		   (static_cast<uint64_t>(Get8(buff, 1)) << 48) |
		   (static_cast<uint64_t>(Get8(buff, 2)) << 40) |
		   (static_cast<uint64_t>(Get8(buff, 3)) << 32) |
		   (static_cast<uint64_t>(Get8(buff, 4)) << 24) |
		   (static_cast<uint64_t>(Get8(buff, 5)) << 16) |
		   (static_cast<uint64_t>(Get8(buff, 6)) << 8) |
		   (static_cast<uint64_t>(Get8(buff, 7)));

}

}

#endif

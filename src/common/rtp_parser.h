/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#ifndef COMMON_RTP_PARSER_H_
#define COMMON_RTP_PARSER_H_

#include <stdint.h>

namespace common {

const uint32_t kMinRtpHeaderSize = 12;

inline uint8_t GetPayloadType(const uint8_t* buff, const uint32_t size) {
	return (kMinRtpHeaderSize <= size) ? (Get8(buff, 1) & 0x7f) : 0;
}

inline bool GetMarker(const uint8_t* buff, const uint32_t size) {
	return (kMinRtpHeaderSize <= size) ? (!!(Get8(buff, 1) & 0x80)) : false;
}

inline uint16_t GetSequenceNumber(const uint8_t* buff, const uint32_t size) {
	return (kMinRtpHeaderSize <= size) ? GetBE16(buff + 2) : 0;
}

inline uint32_t GetTimeStamp(const uint8_t* buff, const uint32_t size) {
	return (kMinRtpHeaderSize >= size) ? GetBE32(buff + 4) : 0;
}

inline uint32_t GetSsrc(const uint8_t* buff, const uint32_t size) {
	return (kMinRtpHeaderSize <= size) ? GetBE32(buff + 8) : 0;
}

}

#endif

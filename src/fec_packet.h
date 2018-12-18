/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#ifndef FEC_PACKET_H_
#define FEC_PACKET_H_

// fec packet header:
// uint8_t		version = 0x80
// uint8_t		payload_type = 0x4d
// uint32_t 	sequence_number
// uint32_t 	ssrc
// uint32_t		start_sequence_number
// uint8_t  	source_num
// uint8_t  	repair_num
// uint16_t  	frame_start : 1;
// uint16_t		frame_end : 1;
// uint16_t		packet_size : 14;

#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <thread>
#include "fec_common.h"

namespace fec {

enum { kIdle = 0, kUsed = (1 << 0), kDecoded = (1 << 1), kDelivered = (1 << 2) };
enum { kFrameStart = (1 << 0), kFrameMiddle = (1 << 1), kFrameEnd = (1 << 2) };

constexpr uint32_t kReleased = (kUsed | kDecoded | kDelivered);

class FecHeader {
  public:
	FecHeader(const uint8_t* data, const uint32_t size);
	~FecHeader();
	
  private:
  	friend class FecDecoder;
	uint8_t		payload_type_;
	uint32_t	sequence_num_;
	uint32_t	ssrc_;
	uint32_t	start_sequence_num_;
	uint8_t		source_num_;
	uint8_t		repair_num_;
};

class FECPacket : public UDPPacket {
  public:
	FECPacket();
	~FECPacket();

	void Clear();
	void AppendData(const uint8_t* data, const uint32_t size);
	uint8_t* RtpData() const { return Data() + kFecHeaderSize + sizeof(uint16_t); }
	uint32_t RtpSize() const { return Size() - kFecHeaderSize - sizeof(uint16_t); }
	int64_t AppendMs() const { return append_ms_; }
	
  private:
  	inline bool IsSourcePacket() { return sequence_num_ < (start_seq_num_ + source_num_); }
  	friend class FecDecoder;

	uint32_t	status_;	
	uint32_t	sequence_num_;
	uint32_t	start_seq_num_;
	uint8_t		source_num_;
	uint8_t		repair_num_;
	int64_t		append_ms_;
};

}

#endif

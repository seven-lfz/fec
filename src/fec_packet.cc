/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#include "fec_packet.h"
#include "byteorder.h"

namespace fec {

FecHeader::FecHeader(const uint8_t* data, const uint32_t size)
	: payload_type_(common::Get8(data, 1) & 0x7f),
	sequence_num_(common::GetBE32(data + 2)),
	ssrc_(common::GetBE32(data + 6)),
	start_sequence_num_(common::GetBE32(data + 10)),
	source_num_(common::Get8(data, 14)),
	repair_num_(common::Get8(data, 15)) {

}

FecHeader::~FecHeader() { }

FECPacket::FECPacket() 
	: UDPPacket(), 
	status_(kIdle),
	sequence_num_(0),
	start_seq_num_(0),
	timestamp_(0),
	frame_flag_(0),
	source_num_(0),
	repair_num_(0) {

}

FECPacket::~FECPacket() { }

void FECPacket::Clear() {
	Reset();

	status_ = kIdle;
}

}


/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#include "byteorder.h"
#include "rtp_parser.h"
#include "fec_encoder.h"

namespace fec {

FecEncoder::FecEncoder(const uint32_t rows, const uint32_t columns, const uint32_t ssrc)
	: ssrc_(ssrc),
	fec_row_num_(rows),
	fec_column_num_(columns),
	start_seq_num_(0),
	current_seq_num_(0) {

}

FecEncoder::~FecEncoder() {

}

FecEncoder* FecEncoder::Create(const uint32_t rows, const uint32_t columns, const uint32_t ssrc) {
	FecEncoder* inst = new FecEncoder(rows, columns, ssrc);
	if (0 == inst->Init())
		return inst;

	delete inst;
	return nullptr;
}

int FecEncoder::OnPacketDelivered(const uint8_t* data, const uint32_t size) {
	if (!IsPacketValid(data, size))
		return -1;
	
	DeliverSourcePacket(data, size);

	if (TimeToDeliverRepair()) {
		DeliverRepairPacket();
		RefreshFecParameter();
	}

	return 0;
}

int FecEncoder::ModifyFecParameter(const uint32_t rows, const uint32_t columns) {
	return 0;
}

int FecEncoder::Init() {
	of_session_t* session = nullptr;

	of_parameters_t param = {fec_row_num_, fec_column_num_, kMaxMtuSize};
	if (0 == of_create_codec_instance(&session, OF_CODEC_REED_SOLOMON_GF_2_M_STABLE, OF_ENCODER, 2) && 
		0 == of_set_fec_parameters(session, &param)) {
		session_.reset(session);
		return 0;
	}

	return -1;
}

void FecEncoder::DeliverSourcePacket(const uint8_t* data, const uint32_t size) {
	uint32_t idx = current_seq_num_ - start_seq_num_;
	UDPPacket* upkt = packet_list_[idx].get();
	if (!upkt) {
		cout << "there is no packet in array to use" << endl;
		return;
	}
	
	uint8_t* buff = upkt->Data();
	WriteFecHeader(buff);

	// write frame description and payload length, this 16bytes will be encoded by fec
	uint16_t value = size | (IsNewFrame(data, size) << 15) | (IsFrameEnd(data, size) << 14);
	common::SetBE16(buff + kFecHeaderSize, value);

	// write payload data
	uint32_t len = kFecHeaderSize + sizeof(uint16_t);
	memcpy(buff + len, data, size);
	len += size;

	if (0 == upkt->SetSize(len)) 
		pkt_sender_(buff, len);
	else
		cout << "fec packet size " << len << " is invalid" << endl;
}

void FecEncoder::DeliverRepairPacket() {
	uint32_t rows = fec_row_num_ + fec_column_num_;
	std::unique_ptr<uint8_t> ptr(new uint8_t[rows * sizeof(uint8_t*)]);
	void** packet_ptrs = (void**)(ptr.get());

	uint32_t max_payload_size = 0;
	for (uint32_t i = 0; i < rows; i++) {
		packet_ptrs[i] = packet_list_[i]->Data() + kFecHeaderSize;

		if (i < fec_row_num_) {
			max_payload_size = std::max(max_payload_size, packet_list_[i]->Size());
			continue;
		}

		of_build_repair_symbol(session_.get(), packet_ptrs, i);
		WriteFecHeader(packet_list_[i]->Data());
		pkt_sender_(packet_list_[i]->Data(), max_payload_size);
	}
}

void FecEncoder::RefreshFecParameter() {
	start_seq_num_ = current_seq_num_;

	
}

bool FecEncoder::IsPacketValid(const uint8_t* data, const uint32_t size) {
	return common::GetSsrc(data, size) == ssrc_;
}

bool FecEncoder::TimeToDeliverRepair() {
	return (start_seq_num_ + fec_row_num_) <= current_seq_num_;
}

bool FecEncoder::IsNewFrame(const uint8_t* data, const uint32_t size) {
	return common::GetTimeStamp(data, size) != last_frame_timestamp_;
}

bool FecEncoder::IsFrameEnd(const uint8_t* data, const uint32_t size) {
	return common::GetMarker(data, size);
}

void FecEncoder::WriteFecHeader(uint8_t* data) { 
	data[0] = 0x80;
	data[1] = kFecPayloadType;
	common::SetBE32(data + 2, current_seq_num_++);
	common::SetBE32(data + 6, ssrc_);
	common::SetBE32(data + 10, start_seq_num_);
	data[14] = fec_row_num_;
	data[15] = fec_column_num_;
}

}

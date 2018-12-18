/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#include "common/byteorder.h"
#include "common/rtp_parser.h"
#include "fec_encoder.h"

namespace fec {

FecEncoder::FecEncoder(PacketSender pkt_sender, const uint32_t rows, const uint32_t columns, const uint32_t ssrc)
	: pkt_sender_(pkt_sender),
	ssrc_(ssrc),
	fec_sources_(rows),
	fec_repairs_(columns) {

}

FecEncoder::~FecEncoder() {

}

FecEncoder* FecEncoder::Create(PacketSender pkt_sender, const uint32_t rows, const uint32_t columns, const uint32_t ssrc) {
	FecEncoder* inst = new FecEncoder(pkt_sender, rows, columns, ssrc);
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

	of_parameters_t param = { fec_sources_, fec_repairs_, (kMaxMtuSize - kFecHeaderSize) };
	if (0 != of_create_codec_instance(&session, OF_CODEC_REED_SOLOMON_GF_2_M_STABLE, OF_ENCODER, 2) || 
		0 != of_set_fec_parameters(session, &param)) 
		return -1;

	session_.reset(session);
	for (uint32_t i = 0; i < (fec_sources_ + fec_repairs_); i++) {
		packet_list_.emplace_back(new UDPPacket());
	}
	
	return 0;
}

void FecEncoder::DeliverSourcePacket(const uint8_t* data, const uint32_t size) {
	uint32_t idx = current_seq_num_ - start_seq_num_;
	UDPPacket* upkt = packet_list_[idx].get();
	if (!upkt) {
		std::cout << "there is no packet in array to use" << std::endl;
		return;
	}
	
	uint8_t* buff = upkt->Data();
	WriteFecHeader(buff);

	// write frame payload length, this 16bytes will be encoded by fec
	common::SetBE16(buff + kFecHeaderSize, static_cast<const uint16_t>(size));

	// write payload data
	uint32_t len = kFecHeaderSize + sizeof(uint16_t);
	memcpy(buff + len, data, size);
	len += size;

	upkt->SetSize(len);
	pkt_sender_(buff, len);
}

void FecEncoder::DeliverRepairPacket() {
	uint32_t rows = fec_sources_ + fec_repairs_;
	std::unique_ptr<uint8_t> ptr(new uint8_t[rows * sizeof(uint8_t*)]);
	void** packet_ptrs = (void**)(ptr.get());

	uint32_t max_payload_size = 0;
	for (uint32_t i = 0; i < rows; i++) {
		packet_ptrs[i] = packet_list_[i]->Data() + kFecHeaderSize;

		if (i < fec_sources_) {
			max_payload_size = std::max(max_payload_size, packet_list_[i]->Size());
			continue;
		}

		of_build_repair_symbol(session_.get(), packet_ptrs, i);
		WriteFecHeader(packet_list_[i]->Data());
		pkt_sender_(packet_list_[i]->Data(), max_payload_size);
	}

	start_seq_num_ = current_seq_num_;
}

void FecEncoder::RefreshFecParameter() {
	// TODO
	return;
}

bool FecEncoder::IsPacketValid(const uint8_t* data, const uint32_t size) {
	return size <= (kMaxMtuSize - kFecHeaderSize - sizeof(uint16_t));
}

bool FecEncoder::TimeToDeliverRepair() {
	return (start_seq_num_ + fec_sources_) <= current_seq_num_;
}

void FecEncoder::WriteFecHeader(uint8_t* data) { 
	data[0] = 0x80;
	data[1] = kFecPayloadType;
	common::SetBE32(data + 2, current_seq_num_++);
	common::SetBE32(data + 6, ssrc_);
	common::SetBE32(data + 10, start_seq_num_);
	data[14] = fec_sources_;
	data[15] = fec_repairs_;
}

}

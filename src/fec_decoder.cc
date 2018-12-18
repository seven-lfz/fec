/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#include "fec_decoder.h"
#include "byteorder.h"
#include "rtp_parser.h"

namespace fec {

FecDecoder::FecDecoder()
	: worker_(std::bind(&FecDecoder::Runnable, this)) {
	
}

FecDecoder::~FecDecoder() {

}

FecDecoder* FecDecoder::Create() {
	FecDecoder* inst = new FecDecoder();
	if (0 == inst->Init())
		return inst;

	delete inst;
	return nullptr;
}

int FecDecoder::Init() {
	for (uint32_t i = 0; i < kFecPacketCount; i++)
		packet_list_.emplace_back(new FECPacket());

	return 0;
}

int FecDecoder::OnPacketReceived(const uint8_t* data, const uint32_t size) {
	FecHeader fec_hdr(data, size);
	if (kFecPayloadType != fec_hdr.payload_type_)
		return pkt_handler_(data, size);
	
	if (IsPacketNeeded(fec_hdr)) {
		uint32_t idx = fec_hdr.sequence_num_ % packet_list_.size();
		SaveFecPacket(idx, data, size);
	}

	return 0;
}

bool FecDecoder::IsPacketNeeded(FecHeader& fec_header) {
	// this sequence packet have been decoded
	if (fec_header.sequence_num_ < last_decoded_sequence_num_)
		return false;
	
	// source packet is needed
	if ((fec_header.start_sequence_num_ + fec_header.source_num_) > fec_header.sequence_num_)
		return true;

	// if received packet number is equal with source number, other packet in this sequence is not needed
	uint32_t stat_idx = fec_header.start_sequence_num_ % packet_list_.size();
	uint32_t total_num = fec_header.source_num_ + fec_header.repair_num_;
	uint32_t recv_count = 0;
	for (uint32_t i = 0; i < total_num; i++) {
		uint32_t idx = GetIndex(stat_idx + i);

		FECPacket* fpkt = packet_list_[idx].get();
		recv_count += ((fpkt->status_ & kUsed) && (fec_header.start_sequence_num_ == fpkt->start_seq_num_)); 
	}

	return fec_header.source_num_ > recv_count;
}

void FecDecoder::SaveFecPacket(const uint32_t index, const uint8_t* data, const uint32_t size) {
	FECPacket* fpkt = packet_list_[index].get();
	if (kIdle != fpkt->status_) {
		std::cout << "index " << index << " is not idle, sequence number " << fpkt->sequence_num_
			      << " start sequence number " << fpkt->start_seq_num_ << " fec parameter [" << fpkt->source_num_
			      << " | " << fpkt->repair_num_ << "]" << std::endl;

		// TODO
	}

	fpkt->AppendData(data, size);
}

void FecDecoder::Runnable() {
	DecodedFecPacket();

	if (0 == DeliverSourcePacket())
		this_thread::sleep_for(chrono::milliseconds(20));
}

void FecDecoder::DecodedFecPacket() {
	uint32_t loop = 0;
	while (loop < packet_list_.size()) {
		uint32_t idx = GetIndex(next_decoded_idx_ + (loop++));
		FECPacket* fpkt = packet_list_[idx].get();
		if (kIdle == fpkt->status_)
			continue;

		uint32_t pkt_count = fpkt->source_num_ + fpkt->repair_num_;
		if (next_decoded_idx_ == (fpkt->start_seq_num_ % packet_list_.size())) {
			DecodedSequence(next_decoded_idx_, pkt_count);
			break;
		}

		// at least one sequence packet have been lost OR have dirty packet 
		std::cout << "between start index " << next_decoded_idx_ << " and end index " << idx
				  << " may loss at least one sequence packet" << std::endl;
		// TODO
	}
}

int FecDecoder::DecodedSequence(const uint32_t start_idx, const uint32_t pkt_count) {
	uint32_t recv_count = 0;
	uint32_t recv_source_count = 0;
	uint32_t source_num = 0;
	uint32_t start_seq_num = 0;
	int64_t last_append_ms = 0;
	
	for (uint32_t i = 0; i < pkt_count; i++) {
		uint32_t idx = GetIndex(start_idx + i);
		FECPacket* fpkt = packet_list_[idx].get();
		if (kIdle == fpkt->status_)
			continue;

		if (0 == recv_count) {
			source_num = fpkt->source_num_;
			start_seq_num = fpkt->start_seq_num_;
		}
	
		if (start_seq_num != fpkt->start_seq_num_) {
			std::cout << "decoded sequence: start index " << start_idx << " index " << idx << " start sequence number"
					  << fpkt->start_seq_num_ << " is not equal with " << start_seq_num << std::endl;
			continue;
		}

		recv_count++;
		recv_source_count += fpkt->IsSourcePacket();
		last_append_ms = std::max(last_append_ms, fpkt->AppendMs());
	}

	// have received enough packet to decoded
	if (recv_count && source_num <= recv_count) {
		// if have all source packet, don't need to decoded
		source_num == recv_source_count ? 0 : Decoded(start_idx, pkt_count, source_num);
		ResetDecoder(start_idx, pkt_count, source_num);

		// have received enough packet, other source packet or repair packet isn't in need
		last_decoded_sequence_num_ = start_seq_num + pkt_count;
		next_decoded_idx_ = GetIndex(start_idx + pkt_count);
		return 0;
	}

	// broken sequence, don't wait any more time, reset and deliver
	// TODO
	return -1;
}

int FecDecoder::Decoded(const uint32_t start_idx, const uint32_t rows, const uint32_t columns) {
	uint32_t start_seq_num = 0;
	for (uint32_t i = 0; i < rows; i++) {
		uint32_t idx = GetIndex(start_idx + i);
		FECPacket* fpkt = packet_list_[idx].get();
		fec_pointer_[i] = fpkt->status_ & kUsed ? (fpkt->Data() + kFecHeaderSize) : nullptr;
	}
	
	if (0 != of_set_available_symbols(session_.get(), fec_pointer_)) {
		cout << "openfec set available symbols failed" << endl;
		return -1;
	}
	
	if (0 != of_finish_decoding(session_.get())) {
		cout << "openfec finish decoding failed" << endl;
		return -1;
	}
	
	if (0 != of_get_source_symbols_tab(session_.get(), rtp_pointer_)) {
		cout << "get fec source symbol failed" << endl;
		return -1;
	}

	for (uint32_t i = 0; i < rows; i++) {
		if (fec_pointer_[i])
			continue;
		
		uint32_t idx = GetIndex(start_idx + i);
		FECPacket* fpkt = packet_list_[idx].get();

		// pakcet have been lost
		fpkt->status_ = kUsed;
		fpkt->sequence_num_ = start_seq_num + i;
		fpkt->start_seq_num_ = start_seq_num;
		fpkt->source_num_ = columns;
		fpkt->repair_num_ = rows - columns;

		// recover fec source packet
		if (i < columns && rtp_pointer_[i]) {
			uint16_t size = common::GetBE16(rtp_pointer_[i]);
			fpkt->frame_flag_ = ((size >> 15) ? kFrameStart : 0) | ((size & 0x4000) ? kFrameEnd : 0);
			fpkt->timestamp_ = common::GetTimeStamp((uint8_t*)rtp_pointer_[i] + sizeof(uint16_t), size);

			// real size
			size = (size & 0x3fff) + sizeof(uint16_t);
			memcpy(fpkt->Data() + kFecHeaderSize, rtp_pointer_[i], size);
			fpkt->SetSize(size + kFecHeaderSize);

     		// memory free, malloc in of_get_source_symbols_tab function call
			free(rtp_pointer_[i]);
		}
	}

	return 0;
}

void FecDecoder::ResetDecoder(const uint32_t start_idx, const uint32_t rows, const uint32_t columns) {
	session_.reset();

	for (uint32_t i = 0; i < rows; i++) {
		uint32_t idx = GetIndex(start_idx + i);

		FECPacket* fpkt = packet_list_[idx].get();
		if (i < columns) {
			fpkt->status_ |= kDecoded;
			if (kReleased == fpkt->status_)
				fpkt->Clear();
		} else {
			fpkt->status_ = kUsed | kDecoded;
		}
	}
}

int FecDecoder::DeliverSourcePacket() {
	uint32_t loop = 0;
	int result = -1;
	while (loop < packet_list_.size()) {
		FECPacket* fpkt = packet_list_[last_deliver_idx_].get();
		if (kIdle == fpkt->status_)
			break;

		loop++;
		last_deliver_idx_ = GetIndex(last_deliver_idx_ + 1);
		if (fpkt->IsSourcePacket())	 {
			pkt_handler_(fpkt->RtpData(), fpkt->RtpSize());
			result = 0;
		}	
			
	
		fpkt->status_ |= kDelivered;
		if (kReleased == fpkt->status_)
			fpkt->Clear();
	}

	return result;
}

}

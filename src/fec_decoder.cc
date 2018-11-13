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

FecDecoder::FecDecoder(const uint32_t ssrc)
	:ssrc_(ssrc),
	next_decoded_idx_(0),
	last_frame_idx_(0),
	worker_(std::bind(&FecDecoder::Runnable, this)) {

}

FecDecoder::~FecDecoder() {

}

FecDecoder* FecDecoder::Create(const uint32_t ssrc) {
	return new FecDecoder(ssrc);
}

int FecDecoder::OnPacketReceived(const uint8_t* data, const uint32_t size) {
	FecHeader fec_hdr(data, size);
	if (kFecPayloadType != fec_hdr.payload_type_)
		return pkt_handler_(data, size);
	
	if (IsPacketNeeded(fec_hdr)) {
		uint32_t idx = fec_hdr.sequence_num_ % packet_list_.size();
		SaveFecPacket(idx, data, size);

		if (NeedToSetEvent(data, size))
			SetEvent();
	}

	return 0;
}

void FecDecoder::Runnable() {
	if (0 == WaitEvent()) {
		DecodedFecPacket();
		DeliverFrame();
	}
}

bool FecDecoder::IsPacketNeeded(FecHeader& fec_header) {
	// source packet is needed
	if ((fec_header.start_sequence_num_ + fec_header.source_num_) > fec_header.sequence_num_)
		return true;

	// if have received packet number is equal with source number, other packet in this sequence is not needed
	uint32_t stat_idx = fec_header.start_sequence_num_ % packet_list_.size();
	uint32_t total_num = fec_header.source_num_ + fec_header.repair_num_;
	uint32_t recv_count = 0;
	for (uint32_t i = 0; i < total_num; i++) {
		uint32_t idx = (stat_idx + i) & (packet_list_.size() - 1);

		FECPacket* fpkt = packet_list_[idx].get();
		recv_count += ((fpkt->status_ & kUsed) && (fec_header.start_sequence_num_ == fpkt->start_seq_num_)); 
	}

	return fec_header.source_num_ > recv_count;
}

void FecDecoder::SaveFecPacket(const uint32_t index, const uint8_t* data, const uint32_t size) {
	FECPacket* fpkt = packet_list_[index].get();
	if (kIdle != fpkt->status_) {

	}

	fpkt->SetData(data, size);
}

bool FecDecoder::NeedToSetEvent(const uint8_t* data, const uint32_t size) {
	
}

void FecDecoder::SetEvent() {

}

int FecDecoder::WaitEvent() {

}

void FecDecoder::DecodedFecPacket() {
	uint32_t loop = 0;

	while (loop < packet_list_.size()) {
		uint32_t pkt_num = GetNextSequence(next_decoded_idx_);
		if (0 == pkt_num || pkt_num >= packet_list_.size())
			break;

		if (0 != DecodedSequence(next_decoded_idx_, pkt_num))
			break;

		loop += pkt_num;
		next_decoded_idx_ = (next_decoded_idx_ + pkt_num) & (packet_list_.size() - 1);
	}
}

uint32_t FecDecoder::GetNextSequence(const uint32_t start_idx) {
	uint32_t loop = 0;

	while (loop < packet_list_.size()) {
		uint32_t idx = GetIndex(start_idx + (loop++));
		FECPacket* fpkt = packet_list_[idx].get();
		if (!(fpkt->status_ & kUsed))
			continue;

		idx = fpkt->start_seq_num_ % packet_list_.size();
		if (start_idx == idx)
			return fpkt->source_num_ + fpkt->repair_num_;

		// at least one sequence packet have lossed, it will happend?
		cout << "get next sequence: start index " << start_idx << " index " << idx << ", maybe loss one or more sequence"; 
		return start_idx < idx ? (idx - start_idx) : ((idx + packet_list_.size()) - start_idx);
	}

	return packet_list_.size();
}

int FecDecoder::DecodedSequence(const uint32_t start_idx, const uint32_t pkt_count) {
	uint32_t recv_count = 0;
	uint32_t recv_source_count = 0;
	uint32_t source_num = 0;
	uint32_t start_seq_num = 0;
	
	for (uint32_t i = 0; i < pkt_count; i++) {
		uint32_t idx = (start_idx + i) & (packet_list_.size() - 1);

		FECPacket* fpkt = packet_list_[idx].get();
		if (fpkt->status_ & kDecoded) {
			cout << "start index " << start_idx << ", index " << idx << " have been decoded" << endl; 
			break;
		}

		if (fpkt->status_ & kUsed) {
			if (0 == recv_count) {
				source_num = fpkt->source_num_;
				start_seq_num = fpkt->start_seq_num_;
			}

			recv_count += (start_seq_num == fpkt->start_seq_num_);
			recv_source_count += (start_seq_num == fpkt->start_seq_num_ && i < source_num);
		}
	}

	if (recv_count && source_num <= recv_count) {
		source_num == recv_source_count ? 0 : Decoded(start_idx, pkt_count, source_num);
		ResetDecoder(start_idx, pkt_count, source_num);
		return 0;
	}

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

void FecDecoder::DeliverFrame() {
	uint32_t loop = 0;
	
	while (loop < packet_list_.size()) {
		// skip repair packet
		FECPacket* fpkt = packet_list_[last_frame_idx_].get();
		if ((fpkt->status_ & kUsed) && (!fpkt->IsSourcePacket())) {
			loop++;
			last_frame_idx_ = (last_frame_idx_ + 1) & (packet_list_.size() - 1);
			fpkt->Clear();
			continue;
		}

		uint32_t end_idx = GetNextFrame(last_frame_idx_);
		if (packet_list_.size() <= end_idx) {
			ResetBrokenFrame(last_frame_idx_);
			break;
		}

		uint32_t pkt_num = DeliverPacket(last_frame_idx_, end_idx);
		loop += pkt_num;
		last_frame_idx_ = (last_frame_idx_ + pkt_num) & (packet_list_.size() - 1);
	}
}

uint32_t FecDecoder::GetNextFrame(const uint32_t start_idx) {
	uint32_t loop = 0;
	uint32_t timestamp = 0;
	
	while (1) {
		uint32_t end_idx = (start_idx + (loop++)) & (packet_list_.size() - 1);
		FECPacket* fpkt = packet_list_[end_idx].get();
		if (!(fpkt->status_ & kUsed))
			break;

		if (!fpkt->IsSourcePacket())
			continue;

		// find frame first packet
		if (fpkt->frame_flag_ & kFrameStart)
			timestamp = fpkt->timestamp_;

		// find frame end packet
		if (fpkt->frame_flag_ & kFrameEnd)
			return end_idx;

		// if there is a different timestamp between frame start end end packets
		if (timestamp != fpkt->timestamp_) {
			cout << "get next frame. start index " << start_idx << " timestamp is " << timestamp 
				 << ", but index " << end_idx << " timestamp is " << fpkt->timestamp_ << endl;
			break;
		}
		
		if (packet_list_.size() <= loop)
			break;
	}

	return packet_list_.size();
}

uint32_t FecDecoder::DeliverPacket(const uint32_t start_idx, const uint32_t end_idx) {
	uint32_t pkt_num = 0;
	
	while (1) {
		uint32_t idx = (start_idx + (pkt_num++)) & (packet_list_.size() - 1);
		FECPacket* fpkt = packet_list_[idx].get();
		if (!(fpkt->status_ & kUsed))
			continue;

		if (fpkt->IsSourcePacket())
			pkt_handler_(fpkt->RtpData(), fpkt->RtpSize());

		// refresh packet status. if packet have decoded and deliver, release it
		fpkt->status_ |= kDelivered;
		if (kReleased == fpkt->status_)
			fpkt->Clear();
			
		if (idx == end_idx)
			break;
	}

	return pkt_num;
}

void FecDecoder::ResetBrokenFrame(const uint32_t start_idx) {
	
}

}

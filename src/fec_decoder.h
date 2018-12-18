/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#ifndef FEC_DECODER_H_
#define FEC_DECODER_H_

#include <vector>
#include <functional>
#include "fec_packet.h"

namespace fec {

class FecDecoder {
  public:
  	typedef std::function<int(const uint8_t*, const uint32_t)> PacketHandler;
	
	FecDecoder();
	~FecDecoder();
	static FecDecoder* Create();

	int OnPacketReceived(const uint8_t* data, const uint32_t size);
	
  private:
  	int Init();
  	void Runnable();
	bool IsPacketNeeded(FecHeader& fec_header);
	void SaveFecPacket(const uint32_t index, const uint8_t* data, const uint32_t size);
	void DecodedFecPacket();
	int DecodedSequence(const uint32_t start_idx, const uint32_t pkt_count);
	int Decoded(const uint32_t start_idx, const uint32_t rows, const uint32_t columns);
	void ResetDecoder(const uint32_t start_idx, const uint32_t rows, const uint32_t columns);
	int DeliverSourcePacket();

	inline uint32_t GetIndex(const uint32_t index) { return index & (packet_list_.size() - 1); }

	enum { kFecPacketCount = 512 };		// must be 2^n
  	PacketHandler	pkt_handler_;	
	uint32_t	next_decoded_idx_ = 0;
	uint32_t	last_deliver_idx_ = 0;
	uint32_t	last_decoded_sequence_num_ = 0;
	
	void** fec_pointer_ = nullptr;
	void** rtp_pointer_ = nullptr;
	std::thread	worker_;
  	std::unique_ptr<of_session_t, FecSessionDeleter>	session_;
	std::vector<std::unique_ptr<FECPacket>>	packet_list_;
};

}

#endif

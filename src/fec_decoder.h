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
	
	FecDecoder(const uint32_t ssrc);
	~FecDecoder();
	static FecDecoder* Create(const uint32_t ssrc);

	int OnPacketReceived(const uint8_t* data, const uint32_t size);
	
  private:
  	void Runnable();
	bool IsPacketNeeded(FecHeader& fec_header);
	void SaveFecPacket(const uint32_t index, const uint8_t* data, const uint32_t size);
	bool NeedToSetEvent(const uint8_t* data, const uint32_t size);
	void SetEvent();
	int WaitEvent();
	void DecodedFecPacket();
	uint32_t GetNextSequence(const uint32_t start_idx);
	int DecodedSequence(const uint32_t start_idx, const uint32_t pkt_count);
	int Decoded(const uint32_t start_idx, const uint32_t rows, const uint32_t columns);
	void ResetDecoder(const uint32_t start_idx, const uint32_t rows, const uint32_t columns);
	void DeliverFrame();
	uint32_t GetNextFrame(const uint32_t start_idx);
	uint32_t DeliverPacket(const uint32_t start_idx, const uint32_t end_idx);
	void ResetBrokenFrame(const uint32_t start_idx);
	inline uint32_t GetIndex(const uint32_t index) { return index & (packet_list_.size() - 1); }
	
  	PacketHandler	pkt_handler_;
  	const uint32_t ssrc_;
	
	uint32_t	next_decoded_idx_;
	uint32_t	last_frame_idx_;
	std::thread	worker_;
	void** fec_pointer_;
	void** rtp_pointer_;
  	std::unique_ptr<of_session_t, FecSessionDeleter>	session_;
	std::vector<std::unique_ptr<FECPacket>>	packet_list_;
};

}

#endif

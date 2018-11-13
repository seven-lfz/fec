/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#ifndef FEC_ENCODER_H_
#define FEC_ENCODER_H_

#include <vector>
#include <functional>
#include "fec_common.h"

namespace fec {

class FecEncoder {
  public:
  	typedef std::function<int(const uint8_t*, const uint32_t)> PacketSender;
	
	FecEncoder(const uint32_t rows, const uint32_t columns, const uint32_t ssrc);
	~FecEncoder();

	static FecEncoder* Create(const uint32_t rows, const uint32_t columns, const uint32_t ssrc);
	int OnPacketDelivered(const uint8_t* data, const uint32_t size);
	int ModifyFecParameter(const uint32_t rows, const uint32_t columns);
	
  private:
  	int Init();
	void DeliverSourcePacket(const uint8_t* data, const uint32_t size);
	void DeliverRepairPacket();
	void RefreshFecParameter();
	inline bool IsPacketValid(const uint8_t* data, const uint32_t size);
	inline bool TimeToDeliverRepair();
	inline bool IsNewFrame(const uint8_t* data, const uint32_t size);
	inline bool IsFrameEnd(const uint8_t* data, const uint32_t size);
	inline void WriteFecHeader(uint8_t* data);

	PacketSender	pkt_sender_;
	const uint32_t 	ssrc_;
	uint8_t		fec_row_num_;
	uint8_t		fec_column_num_;
  	uint32_t	start_seq_num_;
	uint32_t	current_seq_num_;
	uint32_t	last_frame_timestamp_;
	
	std::unique_ptr<of_session_t, FecSessionDeleter> session_;
	std::vector<std::unique_ptr<UDPPacket>>	packet_list_;
};

}

#endif
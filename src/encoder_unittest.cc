/*
 *
 *
 *  written by seven_lfz@2018
 *
 *
 */

#include "fec_encoder.h"
#include "byteorder.h"


int DeliverPacket(const uint8_t* data, const uint32_t size) {

	return 0;	
}

int main(int argc, char* argv[]) {
	char* pcap_file = argv[1];

	
	FILE* fp = fopen(pcap_file, "rb");
	if (!fp) {
		std::cout << "open pcap file " << pcap_file << " failed" << std::endl;
		return -1;
	}

	// pcap header
	fseek(fp, 24, SEEK_CUR);

	std::unique_ptr<uint8_t[]> buff(new uint8_t[kMaxMtuSize]);
	auto f = std::bind(DeliverPacket, std::placeholders::_1, std::placeholders::_2);
	std::unique_ptr<fec::FecEncoder> encoder(fec::FecEncoder::Create(f, 6, 2, 100000));
	
	uint32_t len = 0;
	const uint32_t kPktHeaderLen = 42;
	while (16 != (len = fread(buff.get(), 1, 16, fp))) {
		uint32_t caplen = common::GetBE32(buff.get() + 12);

		len = fread(buff.get(), 1, caplen, fp);
		if (0 == len || kPktHeaderLen > len) {
			std::cout << "read " << len << " bytes from file, length is invalid" << std::endl;
			break;
		}

		encoder->OnPacketDelivered(buff.get() + kPktHeaderLen, len - kPktHeaderLen);
	}

	fclose(fp);
	return 0;
}

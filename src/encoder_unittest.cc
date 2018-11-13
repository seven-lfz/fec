/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#include "fec_encoder.h"

int DeliverPacket(const uint8_t* data, const uint32_t size) {
	
	
	return 0;
}

int main(int argc, char* argv[]) {

	char* filename = argv[1];

	FILE* fp = fopen(filename, "rb");
	if (!fp) {
		cout << "file " << filename << "is not exist" << endl;
		return -1;
	}

	fseek(fp, 24, SEEK_CUR);
	
	uint8_t buff[kMaxMtuSize];
	memset(buff, 0x0, sizeof(buff));

	uint32_t len = 0;
	while (16 != (len = fread(buff, 1, 16, fp))) {
		uint32_t pkt_len = ((uint32_t*)buff)[3];

		len = fread(buff, 1, pkt_len, fp);
		if (len != pkt_len) {
			cout << "read packet size " << len << " is not equal with length " << pkt_len << endl;
			break;
		}

		uint8_t* rtp_data = buff + 42;
		uint32_t rtp_size = pkt_len - 42;


	}
}

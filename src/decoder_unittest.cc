/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "fec_decoder.h"

int BindSocket(const char* ip, const uint16_t port) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (0 > fd) {
		cout << "socket init failed" << endl;
		return -1;
	}

	struct sockaddr_in local;
	
	if (0 != ::bind(fd, (struct sockaddr*)&local, sizeof(local))) {
		cout << "socket bind failed" << endl;
		return -1;
	}

	return fd;
}

int EncodedPacket(const uint8_t* data, const uint32_t size) {

}

int main(int argc, char* argv[]) {
	char* ip = argv[1];
	uint16_t port = atoi(argv[2]);
	uint32_t ssrc = atoi(argv[3]);
	
	int fd = BindSocket(ip, port);
	if (0 > fd)
		return -1;
	
	std::unique_ptr<fec::FecDecoder> decoder(fec::FecDecoder::Create(ssrc));
	if (!decoder) {
		cout << "create fec decoder failed" << endl;
		close(fd);
		return -1;
	}
	
	std::unique_ptr<uint8_t[]> buff(new uint8_t[kMaxMtuSize]);
	while (1) {
		struct sockaddr_in from;
		socklen_t from_len = sizeof(from);
		int len = ::recvfrom(fd, buff.get(), kMaxMtuSize, 0, (struct sockaddr*)&from, &from_len);
		if (0 >= len)
			break;

		EncodedPacket(buff.get(), len);
	}

	close(fd);
	return 0;
}


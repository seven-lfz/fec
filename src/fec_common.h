/*
 *
 *
 *  copyright by seven_lfz@2018
 *
 *
 */

#ifndef FEC_COMMON_H_
#define FEC_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <memory>

extern "C" {
#include "src/lib_common/of_openfec_api.h"
}

using namespace std;

namespace fec {

#define kFecPayloadType		0x4D
#define kMaxMtuSize			1500
#define kFecHeaderSize		16

struct FecSessionDeleter {
	void operator()(of_session_t* ptr) const { of_release_codec_instance(ptr); }
};

class UDPPacket {
  public:
	UDPPacket()
		: capcity_(kMaxMtuSize),
		size_(0),
		data_(new uint8_t[kMaxMtuSize]) {

	}
	
	~UDPPacket() { }

	uint8_t* Data() const { return data_.get(); }
	uint32_t Size() const { return size_; }
	void Reset() { size_ = 0; }
	int SetSize(const uint32_t size) { 
		if (size <= capcity_) {
			size_ = size;
			return 0;
		}

		return -1;
	}

	void SetData(const uint8_t* data, const uint32_t size) {
		uint32_t len = std::min(capcity_, size);
		memcpy(data_.get(), data, len);
		size_ = len;	
	}

  private:
	const uint32_t capcity_;
	uint32_t size_;
	std::unique_ptr<uint8_t> data_;
};


}

#endif

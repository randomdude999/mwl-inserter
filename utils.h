#pragma once
#include <map>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <exception>
#include <fstream>


std::map<char, int> bitdesc_to_values(const char* data, std::string bitdesc, int offset);
std::string readfile(std::string name, std::ios_base::openmode mode, int offset = 0);
std::string rle1_compress(std::string input);
std::string compress_bg(std::string data);
bool file_exists(std::string name);
float timeDiff(clock_t start, clock_t end);

inline int snestopc(int addr) { return ((addr&0x7F0000)>>1|(addr&0x7FFF)); }
inline int pctosnes(int addr) { return ((addr<<1)&0x7F0000)|(addr&0x7FFF)|0x8000; }
inline uint32_t get_little_endian_int(const char* data) { return data[0] + (data[1]<<8) + (data[2]<<16) + (data[3]<<24); }
inline uint16_t get_little_endian_word(const char* data) { return data[0] + (data[1]<<8); }

template<class BidiIter>
BidiIter random_unique(BidiIter begin, BidiIter end, size_t num_random) {
	size_t left = std::distance(begin, end);
	while(num_random--) {
		BidiIter r = begin;
		std::advance(r, std::rand() % left);
		std::swap(*begin, *r);
		++begin;
		--left;
	}
	return begin;
}

inline bool ends_with(std::string const & value, std::string const & ending) {
	if(ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

#define log(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

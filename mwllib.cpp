#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "mwllib.h"
#include "utils.h"

MWLFile::MWLFile(std::string mwl_data) {
	mwldata = mwl_data;
}

MWLFile::MWLFile(char* fname) {
	mwldata = readfile(fname, std::ios::binary, 0);
}

uint32_t get_little_endian_int(const unsigned char* data) {
	return (uint32_t)data[0] + ((uint32_t)data[1]<<8) + ((uint32_t)data[2]<<16) + ((uint32_t)data[3]<<24);
}

std::vector<std::pair<uint32_t, uint32_t>> MWLFile::get_pointer_indexes() {
	std::vector<std::pair<uint32_t, uint32_t>> out;
	const char* data = mwldata.c_str();
	uint32_t ptr_tbl_start = get_little_endian_int((unsigned char*)data+4);
	uint32_t ptr_tbl_len = get_little_endian_int((unsigned char*)data+8);
	for(unsigned int i = 0; i < (ptr_tbl_len/8); i++) {
		std::pair<uint32_t, uint32_t> p;
		p.first = get_little_endian_int((unsigned char*)data + ptr_tbl_start + i*8);
		p.second = get_little_endian_int((unsigned char*)data + ptr_tbl_start + i*8 + 4);
		out.push_back(p);
	}
	return out;
}

std::string MWLFile::get_section(MWLSection section) {
	std::pair<uint32_t, uint32_t> sec_ptr = get_pointer_indexes().at((int)section);
	return mwldata.substr(sec_ptr.first, sec_ptr.second);
}

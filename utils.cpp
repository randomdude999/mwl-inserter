#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <queue>
#include <ctime>
#include "utils.h"

std::map<char, int> bitdesc_to_values(const char* data, std::string bitdesc, int offset) {
	std::map<char, int> out;
	int bitc;
	int i;
	bitdesc.erase(std::remove_if(bitdesc.begin(), bitdesc.end(), ::isspace), bitdesc.end());
	bitc = bitdesc.length();
	// first pass, do uppercase chars only
	for(i = 0; i < bitc; i++) {
		int cur_byte = i / 8; // current byte
		int cur_bit = 7 - i % 8; // index of the current bit in that byte (LSB=0, MSB=7)
		// trust me the math works
		bool data_bit = (data[cur_byte + offset] & (1 << cur_bit)) >> cur_bit;
		char ch = bitdesc[i];
		if(std::isupper(ch)) {
			ch = std::tolower(ch);
			out[ch] = (out[ch] << 1) | (int)data_bit;
		}
	}
	// second pass, everything else
	for(i = 0; i < bitc; i++) {
		int cur_byte = i >> 3;
		bool data_bit = (data[cur_byte + offset] & (1 << (7 - (i % 8)))) >> (7 - (i % 8));
		char ch = bitdesc[i];
		if(std::isupper(ch) || ch == '-') {
			continue;
		} else {
			out[ch] = (out[ch] << 1) | (int)data_bit;
		}
	}
	return out;
}

std::string readfile(std::string name, std::ios_base::openmode mode, int offset) {
	std::ifstream f(name, mode);
	std::string data;
	if(!f.good()) {
		throw std::string("Could not open file ")+name;
	}
	f.seekg(0, std::ios::end);
	data.resize((size_t)f.tellg()-offset);
	f.seekg(offset, std::ios::beg);
	f.read(&data[0], data.size());
	f.close();
	return data;
}

std::string rle1_compress(std::string input) {
	if(input.size() == 0) {
		return std::string("\xFF\xFF");
	}
	char cur_rle_char;
	int cur_rle_len = 0;
	std::string output;
	std::string dc_run;
	for(char& c : input) {
		if(cur_rle_len && c != cur_rle_char) {
			// we have a different char, write out the RLE run
			// if the RLE run is < 3 chars, add it to the DC buffer
			// otherwise, write out the current DC run and then the current RLE run
			if(cur_rle_len < 3) {
				// add to DC buffer
				if(dc_run.size() + cur_rle_len <= 128) {
					// we can fit it in the current DC run
					dc_run += std::string(cur_rle_len, cur_rle_char);
				} else {
					// too large to put in current DC run, start a new one
					output += (char)dc_run.size() - 1;
					output += dc_run;
					dc_run = std::string(cur_rle_len, cur_rle_char);
				}
			} else {
				// the run is longer, so use a RLE command
				if(dc_run.size()) {
					// pending DC run, write that out first
					output += (char)dc_run.size() - 1;
					output += dc_run;
					dc_run = "";
				}
				output += cur_rle_len-1 | 0x80;
				output += cur_rle_char;
			}
			cur_rle_len = 1;
			cur_rle_char = c;
		} else if(cur_rle_len) {
			// have a RLE run but the char is unchanged
			if(cur_rle_len < ((cur_rle_char == '\xFF') ? 127 : 128)) {
				// the RLE run can fit another char
				cur_rle_len++;
			} else {
				// the RLE run can't fit another char, write it out
				if(dc_run.size()) {
					// pending DC run, write that out first
					output += (char)dc_run.size() - 1;
					output += dc_run;
					dc_run = "";
				}
				output += cur_rle_len-1 | 0x80;
				output += cur_rle_char;
				cur_rle_len = 1;
				cur_rle_char = c;
			}
		} else {
			// cur_rle_len is never 0 except for the first character
			cur_rle_len = 1;
			cur_rle_char = c;
		}
	}
	if(cur_rle_len < 3) {
		// add to DC buffer
		if(dc_run.size() + cur_rle_len <= 128) {
			// we can fit it in the current DC run
			dc_run += std::string(cur_rle_len, cur_rle_char);
			output += (char)dc_run.size() - 1;
			output += dc_run;
		} else {
			// too large to put in current DC run
			output += (char)dc_run.size() - 1;
			output += dc_run;
			output += cur_rle_len-1 | 0x80;
			output += cur_rle_char;
		}
	} else {
		// the run is longer, so use a RLE command
		if(dc_run.size()) {
			// pending DC run, write that out first
			output += (char)dc_run.size() - 1;
			output += dc_run;
		}
		output += cur_rle_len-1 | 0x80;
		output += cur_rle_char;
	}
	output += "\xFF\xFF";
	return output;
}

std::string compress_bg(std::string data) {
	std::string new_data;
	for(size_t i = 0; i < data.size(); i += 2) {
		new_data += data[i];
	}
	for(size_t i = 1; i < data.size(); i += 2) {
		new_data += data[i];
	}
	return rle1_compress(new_data);
}

bool file_exists(std::string name) {
	std::ifstream f(name);
	return f.good();
}

float timeDiff(clock_t start, clock_t end) {
	return (float)(end - start) / CLOCKS_PER_SEC;
}

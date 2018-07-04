#pragma once
#include <string>
#include <vector>

enum class MWLSection {
	level = 0,
	layer1 = 1,
	layer2 = 2,
	sprite = 3,
	palette = 4,
	entrance = 5,
	exanim = 6,
	bypass = 7
};
class MWLFile {
public:
	std::string mwldata;
	MWLFile(std::string mwldata);
	MWLFile(char* fname);

	std::vector<std::pair<uint32_t, uint32_t>> get_pointer_indexes();
	std::string get_section(MWLSection section);
};

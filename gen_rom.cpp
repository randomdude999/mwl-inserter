#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <fstream>
#include "gen_rom.h"
#include "mwllib.h"
#include "utils.h"
#include "asardll.h"

char secondary_entrace_template[] = R"(
; Auto-generated secondary entrance code
!t = %d ; target ID
org read3($0DE191)+!t : db %d
org read3($0DE198)+!t : db %d
org read3($0DE19F)+!t : db %d
org read3($05DC81)+!t : db %d
)";

std::pair<std::map<int, int>, std::string> parse_secondary_exits(int lvlnum, MWLFile& mwl_f) {
	std::map<int, int> out;
	int entr_count = 0;
	std::string patch;
	std::string entrance_data = mwl_f.get_section(MWLSection::entrance).substr(8);
	for(size_t i = 0; i < entrance_data.length(); i += 8) {
		std::map<char, int> entr = bitdesc_to_values(entrance_data.c_str(), "dddddddd -------D aaaaaaaa bbbbbbbb cccccccc", i);
		int entr_id = entr['d'];
		int t_id = entr_count + lvlnum*16;
		out[entr_id] = t_id;
		entr_count++;
		char* buf = (char*)malloc(sizeof(secondary_entrace_template)+32);
		snprintf(buf, sizeof(secondary_entrace_template)+32, secondary_entrace_template, t_id, lvlnum&0xFF, entr['a'], entr['b'], entr['c']);
		patch += buf;
	}
	return std::make_pair(out, patch);
}

std::string replace_screen_exits(std::string layer1_data, std::map<int, int> exits_map, std::map<int, int> levels_map) {
	size_t i = 5; // skip primary level header
	std::string new_data = layer1_data.substr(0, 5); // add primary level header to output too
	while(i < layer1_data.length()) {
		if(layer1_data[i] == '\xFF') {
			new_data += '\xFF';
			break;
		}
		if(i+3 > layer1_data.length()) {
			// invalid MWL! (probably? In any case it can't fit another screen exit)
			new_data += layer1_data.substr(i); // copy rest of the data
			break;
		}
		std::map<char, int> obj_bits = bitdesc_to_values(layer1_data.c_str(), "nBByyyyy bbbbxxxx ssssssss", i);
		std::string obj_data = layer1_data.substr(i, 3);
		i += 3;
		int obj_num = obj_bits['b'];
		int obj_sett = obj_bits['s'];

		if(obj_num == 0x00 && obj_sett == 0x02) {
			// extended screen exit
			auto flags = bitdesc_to_values(layer1_data.c_str(), "dddddddd DDDDw--D", i);
			i += 2;
			int dest = flags['d'];
			int target = exits_map.at(dest);
			new_data += obj_data[0];
			new_data += flags['w']*8 + 0b0110;
			new_data += (char)0;
			new_data += target;
		} else if(obj_num == 0x00 && obj_sett == 0x00) {
			// normal screen exit
			auto flags = bitdesc_to_values(layer1_data.c_str(), "----wush", i-2);
			int dest = layer1_data[i];
			i += 1;
			if(!flags['s']) {
				// normal level exit, just replace level number
				new_data += obj_data;
				new_data += levels_map.at(dest);
			} else {
				// secondary exit
				int target = exits_map[dest];
				if(target > 0xFF) {
					new_data += obj_data[0];
					new_data += obj_data[1]|1; // force set the `h` in `wush`
					new_data += obj_data[2];
					new_data += target & 0xFF;
				} else {
					new_data += obj_data[0];
					new_data += obj_data[1]&0x0E;
					new_data += obj_data[2];
					new_data += target;
				}
			}
		} else {
			// any other L1 object
			new_data += obj_data;
		}

		int extra_data_c = 0;
		switch(obj_num) {
		case 0x2D: // custom object
			extra_data_c = 2;
			break;
		case 0x22: // direct map16 (page 0)
			extra_data_c = 1;
			break;
		case 0x23: // direct map16 (page 1)
			extra_data_c = 1;
			break;
		case 0x27: // direct map16
			switch(layer1_data[i] & 0xC0) {
			case 0x00: // single screen, single tile
				extra_data_c = 2;
				break;
			case 0x40: // multiple tiles unstreched
				extra_data_c = 2;
				break;
			case 0x80: // single screen, multiple tiles
				extra_data_c = 3;
				break;
			case 0xC0: // multi-screen
				extra_data_c = 4;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		new_data += layer1_data.substr(i, extra_data_c);
		i += extra_data_c;
	}
	return new_data;
}

std::string insert_mwl(int lvlnum, MWLFile& mwl, std::string rom, std::map<int,int> secondary_entr_map, std::map<int, int> levelnum_map) {
	log("insert_mwl %x", lvlnum);
	std::string new_l1_data = replace_screen_exits(mwl.get_section(MWLSection::layer1).substr(8), secondary_entr_map, levelnum_map);
	std::string leveldat = mwl.get_section(MWLSection::level).substr(2, 5) + mwl.get_section(MWLSection::layer2)[0];
	std::string l2_data;
	log("Layer2 first byte: %x", mwl.get_section(MWLSection::layer2)[0]);
	if(mwl.get_section(MWLSection::layer2)[0] != 0) { // i'm afraid this is a slight hack.
		// uncompressed map16 tilemap, need to compress
		l2_data = compress_bg(mwl.get_section(MWLSection::layer2).substr(8));
		leveldat[5] = leveldat[5] | 0x06; // set the bit that uses the LM format not the vanilla one, and also the BG vs OBJ bit
		// ^ is this line needed? I have noticed that for BG data it's always set anyways...
		log("L2 is BG");
	} else {
		// object data, no compression
		l2_data = mwl.get_section(MWLSection::layer2).substr(8);
		log("L2 is OBJ");
	}
	std::string sprite_data = mwl.get_section(MWLSection::sprite).substr(8);
	log("L1 len: %lu, L2 len: %lu, sprite len: %lu", new_l1_data.size(), l2_data.size(), sprite_data.size());
	patchparams pp;
	memset(&pp, 0, sizeof(pp));
	pp.structsize = sizeof(pp);
	pp.patchloc = "lvl_insert_base.asm";
	char* romdata = (char*)malloc(asar_maxromsize());
	pp.romdata = romdata;
	memcpy(pp.romdata, rom.c_str(), rom.length());
	pp.buflen = asar_maxromsize();
	int romlen = rom.length();
	pp.romlen = &romlen;
	pp.should_reset = true;
	memoryfile memory_f[] = {
		{ "layer1.bin", (void*)new_l1_data.c_str(), new_l1_data.length() },
		{ "layer2.bin", (void*)l2_data.c_str(), l2_data.length() },
		{ "sprite.bin", (void*)sprite_data.c_str(), sprite_data.length() },
		{ "secondary.bin", (void*)leveldat.c_str(), leveldat.length() }
	};
	pp.memory_files = memory_f;
	pp.memory_file_count = sizeof(memory_f) / sizeof(memoryfile);
	char buf[32];
	snprintf(buf, 32, "%d", lvlnum);
	definedata defs[] = { {"lvlnum", buf} };
	pp.additional_defines = defs;
	pp.additional_define_count = 1;
	clock_t start_t = clock();
	if(!asar_patch_ex(&pp)) {
		int errdata_count;
		const errordata* errs = asar_geterrors(&errdata_count);
		std::string err_text;
		for(int i = 0; i < errdata_count; i++) {
			err_text += errs[i].fullerrdata;
			err_text += "\n";
		}
		throw err_text;
	}
	int blockc;
	const writtenblockdata* blocks = asar_getwrittenblocks(&blockc);
	log("[insert_mwl] Successfully ran Asar (wrote %d blocks, took %f seconds)", blockc, timeDiff(start_t, clock()));
	// for(int i = 0; i < blockc; i++) {
	//	log("  Written block @ $%X len $%X\n", blocks[i].snesoffset, blocks[i].numbytes);
	//}
	std::string out = std::string(romdata, romlen);
	free(romdata);
	return out;
}

std::string insert_lvl_and_sub(int startnum, std::string romdata, MWLFile& mwl, bool has_sub, MWLFile* submwl) {
	log("insert_mwl_and_sub %d", startnum);
	auto asdf = parse_secondary_exits(startnum, mwl);
	std::map<int, int> secondary_entr_map = asdf.first;
	std::string secondary_entr_patch = asdf.second;
	int main_src = get_little_endian_word(mwl.get_section(MWLSection::level).c_str());
	if(has_sub) {
		auto asdf2 = parse_secondary_exits(startnum+10, *submwl);
		secondary_entr_map.insert(asdf2.first.begin(), asdf2.first.end());
		secondary_entr_patch += asdf2.second;
		int sub_src = get_little_endian_word(submwl->get_section(MWLSection::level).c_str());
		std::map<int, int> levelnum_map = {
			{ main_src, startnum },
			{ sub_src, startnum+10 }
		};
		romdata = insert_mwl(startnum, mwl, romdata, secondary_entr_map, levelnum_map);
		romdata = insert_mwl(startnum+10, *submwl, romdata, secondary_entr_map, levelnum_map);
	} else {
		std::map<int, int> levelnum_map = {
			{ main_src, startnum }
		};
		romdata = insert_mwl(startnum, mwl, romdata, secondary_entr_map, levelnum_map);
	}
	patchparams pp;
	memset(&pp, 0, sizeof(pp));
	pp.structsize = sizeof(pp);
	pp.patchloc = "secondary_entr_patch.asm";
	pp.romdata = (char*)malloc(asar_maxromsize());
	memcpy(pp.romdata, romdata.c_str(), romdata.length());
	pp.buflen = asar_maxromsize();
	int romlen = romdata.length();
	pp.romlen = &romlen;
	pp.numincludepaths = 0;
	pp.should_reset = true;
	pp.additional_define_count = 0;
	pp.stddefinesfile = nullptr;
	pp.stdincludesfile = nullptr;
	pp.memory_file_count = 1;
	memoryfile m_files[] = {
		{ "secondary_entr_patch.asm", (void*)secondary_entr_patch.c_str(), secondary_entr_patch.length() }
	};
	pp.memory_files = m_files;
	pp.warning_setting_count = 0;
	clock_t start_t = clock();
	if(!asar_patch_ex(&pp)) {
		int errdata_count;
		const errordata* errs = asar_geterrors(&errdata_count);
		std::string err_text;
		for(int i = 0; i < errdata_count; i++) {
			err_text += errs[i].fullerrdata;
			err_text += "\n";
		}
		throw err_text;
	} else {
		int blockc;
		const writtenblockdata* blocks = asar_getwrittenblocks(&blockc);
		log("[secondary_entr] Successfully ran Asar (wrote %d blocks, took %f secs)", blockc, timeDiff(start_t, clock()));
		// for(int i = 0; i < blockc; i++) {
		//	log("  Written block @ $%X len %X\n", blocks[i].snesoffset, blocks[i].numbytes);
		//}
		romdata = std::string(pp.romdata, *pp.romlen);
		free(pp.romdata);
	}
	return romdata;
}

std::string generate_10lvl_rom(std::vector<std::string> choices) {
	log("Opening base rom");
	std::string rom = readfile("smw_maker_base_10lvl.smc", std::ios::binary, 512);
	log("loaded base rom, size: %lu ($%lX)", rom.size(), rom.size());

	clock_t all_start = clock();
	for(int i = 0; i < 10; i++) {
		clock_t this_start = clock();
		log("Inserting level %d (ID %s)", i, choices[i].c_str());
		std::string id = choices[i];
		char main_mwl_name[256];
		snprintf(main_mwl_name, 256, "levels/%s_main.mwl", id.c_str());
		MWLFile main_mwl = MWLFile(main_mwl_name);

		char sub_mwl_name[256];
		snprintf(sub_mwl_name, 256, "levels/%s_sub.mwl", id.c_str());
		if(file_exists(sub_mwl_name)) {
			MWLFile sub_mwl = MWLFile(sub_mwl_name);
			rom = insert_lvl_and_sub(i+1, rom, main_mwl, true, &sub_mwl);
		} else {
			rom = insert_lvl_and_sub(i+1, rom, main_mwl, false, nullptr);
		}
		log("Successfully inserted level %d (took %f seconds)", i, timeDiff(this_start, clock()));
	}
	log("Inserted all level files (total %f seconds)", timeDiff(all_start, clock()));
	log("Out rom size: %lu ($%lX)", rom.size(), rom.size());

	return rom;
}

std::string generate_1lvl_rom(std::string id) {
	std::string rom = readfile("smw_maker_base_1lvl.smc", std::ios::binary, 512);
	log("loaded base rom, size: %lu ($%lX)", rom.size(), rom.size());

	clock_t start_t = clock();
	char main_mwl_name[256];
	snprintf(main_mwl_name, 256, "levels/%s_main.mwl", id.c_str());
	MWLFile main_mwl = MWLFile(main_mwl_name);

	char sub_mwl_name[256];
	snprintf(sub_mwl_name, 256, "levels/%s_sub.mwl", id.c_str());
	if(file_exists(sub_mwl_name)) {
		MWLFile sub_mwl = MWLFile(sub_mwl_name);
		rom = insert_lvl_and_sub(1, rom, main_mwl, true, &sub_mwl);
	} else {
		rom = insert_lvl_and_sub(1, rom, main_mwl, false, nullptr);
	}
	log("Sucessfully inserted 1 lvl (%f seconds)", timeDiff(start_t, clock()));
	log("Out rom size: %lu ($%lX)", rom.size(), rom.size());
	return rom;
}

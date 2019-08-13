#include "mwllib.h"
#include "utils.h"
#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

class translevel {
public:
    std::vector<MWLFile> rooms;
    //std::map<int, int> secondary_entrances;
    std::string level_name;
    std::string message_box_1;
    std::string message_box_2;
	int main_room;
	// key is the number of the exgfx file, value is the binary contents of file
	std::map<short, std::string> gfx_files;
};

// returns a vector of pairs. first item in pair is filename, 2nd is "is directory" flag
std::vector<std::pair<std::string,bool>> list_files_in_dir(std::string dirname) {
	std::vector<std::pair<std::string,bool>> out;
#ifdef _WIN32
	dirname += "\\*";
 	WIN32_FIND_DATA find_data;	
	HANDLE findHandle = FindFirstFile(dirname.c_str(), &find_data);
	if(findHandle == INVALID_HANDLE_VALUE) {
		return out; // return empty vector, specifying "no files found"
	}
	do {
		out.push_back(std::make_pair(find_data.cFileName,find_data.dwFileAttribute&FILE_ATTRIBUTE_DIRECTORY));
	} while(FindNextFile(findHandle, &find_data));

 	FindClose(findHandle);
#else
	dirent* ent;
	DIR* dir = opendir(dirname.c_str());
	if(dir == nullptr) return out;
	while((ent = readdir(dir)) != nullptr) {
		if(ent->d_type != DT_REG || ent->d_type != DT_DIR) continue;
		out.push_back(std::make_pair(ent->d_name,ent->d_type==DT_DIR));
	}
#endif
	return out;
}

/* expected structure:
     levels/
       level_001/
        main.mwl
        more.mwl
        gfx123.bin
        exdata.txt
      level_002/
        ...
 */
std::vector<translevel> load_levels_folder(char* folder_name) {
	std::vector<translevel> out;
	auto dir_listing = list_files_in_dir(folder_name);
	for(auto &folder : dir_listing) {
		if(!folder.second) continue; // skip non-directories
		std::string name = folder.first;
		translevel tlvl;
		std::map<short, std::string> gfx_files;
		auto level_folder = list_files_in_dir(name);
		for(auto &file : level_folder) {
			if(file.second) continue; // skip subdirectories
			std::string fname = file.first;
			if(ends_with(fname, ".mwl")) {
				// handle mwl level
				MWLFile this_lvl = MWLFile((name+"/"+fname).c_str());
				tlvl.rooms.push_back(this_lvl);
			} else if(ends_with(fname, ".bin") && (starts_with(fname, "gfx") || starts_with(fname, "GFX"))) {
				// GFX file
				std::string number = fname;
				number.erase(0, 3);
				number.erase(number.end()-4, number.end());
				bool valid_name;
				for(char ch : number) if(!isxdigit(ch)) valid_name = false;
				if(number.size()>3) valid_name=false;
				if(valid_name) {
					short gfx_num = strtol(number.c_str(), nullptr, 16);
					std::string file_contents = readfile(name+"/"+fname, std::ios::binary, 0);
					gfx_files[gfx_num] = file_contents;
				}
			} else if(fname == "exdata.txt") {
				// handle extra data file
				/* format:
				name: LEVELNAME (up to whatever smw's limit was)
				main_room: main.mwl (filename)
				message_box_1:
				HELLO! THIS IS A
				MESSAGE BOX.
				(8 rows of this)
				message_box_2:
				(see above)
				*/
				std::ifstream f(name+"/"+fname);
				for(std::string line; std::getline(f, line); ) {
					std::string key = line.substr(0, line.find(':'));
					if(key == "message_box_1" || key == "message_box_2") {
						std::string out;
						std::string message_line;
						for(int i = 0; i < 8; i++) {
							std::getline(f, message_line);
							// pad message line to 18 characters
							size_t line_length = message_line.length();
							if(line_length < 18) {
								message_line.append(18-line_length, ' ');
							} else if(line_length > 18) {
								//message_line = message_line.substr(0, 18);
								throw "Message box line too long.";
							}
							out.append(message_line);
							out.push_back('\n');
						}
						if(key == "message_box_1") tlvl.message_box_1 = out;
						else tlvl.message_box_2 = out;
					} else if(key == "name") {
						std::string levelname = line.substr(line.find(':')+1);
						if(levelname.length() < 19) {
							name.append(19-levelname.length(), ' ');
						} else if(levelname.length() > 19) {
							//levelname = levelname.substr(0, 19);
							throw "Level name too long.";
						}
						tlvl.level_name = levelname;
					} else if(key == "main_room") {
						// a bit of a waste to reload the file, but it doesn't matter
						std::string room_mwl_name = line.substr(line.find(':')+1);
						MWLFile main_room_lvl((name+"/"+room_mwl_name).c_str());
						int lvlnum = get_little_endian_word(main_room_lvl.get_section(MWLSection::level).substr(0, 2).c_str());
						tlvl.main_room = lvlnum;
					} else {
						throw "Invalid key in exdata.txt.";
					}
				}
			}
		}
		out.push_back(tlvl);
	}
	return out;
}

/* expected structure:
     levels/
       level_001.mwl
       level_001.txt
       level_002.mwl
       level_002.txt
       gfx123.bin
 */
std::vector<translevel> load_levels_flat(char* folder_name) {
    
}
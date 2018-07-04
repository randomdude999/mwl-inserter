#include <ctime>
#include <cstdlib>
#include <cstdio>
#include "gen_rom.h"
#include "asardll.h"
#include "utils.h"
#include <unistd.h>

// thanks to:
// Vitor Vilela and Alcaro for misc coding help
// Baserom patches:
// Vitor Vilela (SA-1)
// kaizoman/Thomas (death counter)
// Noobish Noobsicle (one file, one player)
// p4plus2 (1 or 2 players only)
// Alcaro (no overworld)
// Also Alcaro for writing Flips and Asar

int main(int argc, char** argv) {
	clock_t start_time = clock();
	std::string rom;
	srand((unsigned int)time(nullptr));
	if(!asar_init()) {
		fprintf(stderr, "Could not load Asar DLL");
		return 1;
	}
	if (argc == 11) {
		log("Generating 10lvl rom");
		try {
			std::vector<std::string> args;
			for(int i = 1; i < argc; i++) {
				args.push_back(argv[i]);
			}
			rom = generate_10lvl_rom(args);
		} catch(std::string err) {
			fprintf(stderr, "%s\n", err.c_str());
			return 1;
		}
	}
	else if (argc == 2) {
		log("Generating 1lvl rom %s", argv[1]);
		try {
			rom = generate_1lvl_rom(argv[1]);
		} catch(std::string err) {
			fprintf(stderr, "%s\n", err.c_str());
			return 1;
		}
	}
	else {
		fprintf(stderr, "Error: invalid number of arguments\n");
		return 1;
	}
	log("Total: took %f seconds.", timeDiff(start_time, clock()));
	if(isatty(fileno(stdout))) {
		fprintf(stderr, "Won't write binary garbage to terminal.\n");
	} else {
		fwrite(rom.data(), 1, rom.size(), stdout);
	}
	return 0;
}

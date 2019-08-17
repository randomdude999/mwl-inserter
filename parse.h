#include "utils.h"
#include "mwllib.h"

class layer_object {
public:
    int object_id;
    int x;
    int y;
    bool next_screen;
    uint8_t settings;
    std::vector<uint8_t> extra_data;
};

class sprite {
public:
    int x;
    int y;
    int screen;
    int spr_id;
    int extra_bits;
    std::vector<uint8_t> extra_bytes;
};

class level_info {
public:
	uint16_t level_number;

    // 5 bytes from the start of layer 1 data
    uint8_t primary_level_header[5];

    // bytes from $05F000, $05F200, $05F400, $05F600, $05DE00,
    // $06FC00, $06FE00, read3(read3($05D9A2)+70), in that order.
    // before LM3, only 5 bytes of this are used
    uint8_t secondary_level_header[8];

    // from $0EF310
    uint8_t layer_2_settings;

    // from read3(read3($05D9E4)+$0A)
    uint8_t midway_entrance[4];

    // the first byte of the sprite data
    uint8_t sprite_header;

    // from $03FE00, stored in the header for the exanimation section
    uint8_t exanimation_settings;
};

class secondary_entrance {
public:
    // bytes from $05FA00, $05FC00, $05FE00, read3($05DC86), read3($05DC8B),
    // in that order.
    // note: 4th bit of last byte is ignored when writing (it's the destination
    // level's high bit)
    uint8_t settings[5];

    // only used to determine what links to this entrance
    uint16_t id;
};

class local_exanimation {
    // TODO once i figure out how the exanimation format actually works
};

class level {
public:
    std::vector<layer_object> layer1;

    bool layer2_is_bg;
    std::vector<layer_object> layer2_obj;
    // just a raw tilemap
    std::string layer2_bg;

    std::vector<sprite> sprites;

    bool has_custom_palette;
    std::string custom_palette;

    level_info info;

    std::vector<secondary_entrance> secondary_entrances;

    // std::vector<local_exanimation> exanimations;
    // and some shit for the flags and things

    std::vector<uint16_t> gfx_overrides;

    level(MWLFile input, std::string extra_byte_counts);
};
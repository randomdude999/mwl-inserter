#include "utils.h"
#include "mwllib.h"
#include "parse.h"

std::vector<layer_object> parse_layer_data(std::string data) {
    int datalen = data.length();
    std::vector<layer_object> out;
    for(int i = 0; i < datalen; ) {
        if(data[i] == 0xFF) {
            break;
        }
        std::map<char, int> desc = bitdesc_to_values(data.c_str(), "nBByyyyy bbbbxxxx", i);
        layer_object obj;
        obj.x = desc['x'];
        obj.y = desc['y'];
        obj.next_screen = desc['n'];
        obj.object_id = desc['b'];
        obj.settings = data[i+2];
        i += 3;
        int extra_byte_count = 0;
        if(obj.object_id == 0 && obj.settings == 0) {
            // screen exit
            extra_byte_count = 1;
        } else if(obj.object_id == 0x22 || obj.object_id == 0x23) {
            // DM16 - pages 0/1
            extra_byte_count = 1;
        } else if(obj.object_id == 0x27 || obj.object_id == 0x29) {
            // DM16 - larger pages, multi-screen, etc
            switch (data[i] & 0xC0) {
            case 0x00:
            case 0x40:
                extra_byte_count = 2;
                break;
            case 0x80:
                extra_byte_count = 3;
                break;
            case 0xC0:
                extra_byte_count = 4;
                break;
            }
        } else if(obj.object_id == 0x2D) {
            // custom object
            extra_byte_count = 2;
        }
        else if(obj.object_id == 0 && obj.settings == 0x02) {
            // screen exit expansion
            extra_byte_count = 2;
        }
        std::vector<uint8_t> extra_data(data.begin() + i, data.begin() + i + extra_byte_count);
        i += extra_byte_count;
    }
}

level::level(MWLFile input, std::string extra_byte_counts) {
    std::string info_section = input.get_section(MWLSection::level);
    // layout:
    // 0: source level number (2 bytes)
    // 2: part of secondary level header (5 bytes)
    // 7: unused (2 bytes)
    // 9: midway data (4 bytes)
    // 13: unused (1 byte)
    // 14: more secondary level header (3 bytes)
    info.level_number = get_little_endian_word(info_section[0]);
    for(int i = 0; i < 5; i++)
        info.secondary_level_header[i] = info_section[2+i];
    for(int i = 0; i < 4; i++)
        info.midway_entrance[i] = info_section[9+i];
    for(int i = 0; i < 3; i++)
        info.secondary_level_header[5+i] = info_section[14+i];
    
    std::string layer_1_data = input.get_section(MWLSection::layer1);
    has_custom_palette = (layer_1_data[0] & 1);
    for(int i = 0; i < 5; i++)
        info.primary_level_header[i] = layer_1_data[8+i];
    layer1 = parse_layer_data(layer_1_data.substr(8+5));

    std::string layer_2_data = input.get_section(MWLSection::layer2);
    info.layer_2_settings = layer_2_data[0];
    if(info.layer_2_settings != 0) {
        layer2_is_bg = true;
        layer2_bg = layer_2_data.substr(8);
    } else {
        layer2_is_bg = false;
        layer2_obj = parse_layer_data(layer_2_data.substr(8+5));
    }

    if(layer_1_data[0] & 1) {
        has_custom_palette = true;
        custom_palette = input.get_section(MWLSection::palette).substr(8);
    } else {
        has_custom_palette = false;
    }

    std::string secondary_entrance_data = input.get_section(MWLSection::entrance);
    int entrance_count = (secondary_entrance_data.length() - 8) / 8;

    // TODO: sprites, secondary entrances, exanimation, gfx overrides
}
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

std::map<int, uint8_t> parse_extra_byte_table(std::string extra_byte_counts) {
    int index;
    std::map<int, uint8_t> out;
    for(uint8_t c : extra_byte_counts) {
        out[index] = c - 3;
    }
    return out;
}

std::vector<sprite> parse_sprite_data(std::string data, std::map<int, uint8_t> extra_byte_counts, bool new_system) {
    int y_upper_bits = 0;
    int data_length = data.length();
    std::vector<sprite> out;
    for(int i = 0; i < data_length; ) {
        if(data[i] == 0xFF) {
            if(!new_system) break;
            if(data[i+1] == 0xFF) {
                i += 1;
            } else if(data[i+1] == 0xFE) {
                break;
            } else if(data[i+1] < 0x80) {
                y_upper_bits = data[i+1];
                i += 2;
                continue;
            } else {
                throw "invalid additional command in sprite data";
            }
        }
        std::map<char, int> bits = bitdesc_to_values(data.c_str(), "yyyyeeSY xxxxssss nnnnnnnn", i);
        i += 3;
        sprite spr;
        spr.x = bits['x'];
        spr.y = bits['y'] + (y_upper_bits << 5);
        spr.extra_bits = bits['e'];
        spr.screen = bits['s'];
        spr.spr_id = bits['n'];
        int extra_byte_c = extra_byte_counts[spr.spr_id + (spr.extra_bits << 8)];
        spr.extra_bytes = std::vector<uint8_t>(data.begin() + i, data.begin() + i + extra_byte_c);
        i += extra_byte_c;
        out.push_back(spr);
    }
    return out;
}

std::vector<secondary_entrance> parse_secondary_entrances(std::string data) {
    int entrance_count = (data.length() - 8);
    std::vector<secondary_entrance> out;
    for(int i = 0; i < entrance_count; i++) {
        std::string entrance_data = data.substr(i, 8);
        secondary_entrance entr;
        entr.id = get_little_endian_word(entrance_data.c_str());
        for(int i = 0; i < 5; i++)
            entr.settings[i] = entrance_data[2+i];
        out.push_back(entr);
    }
    return out;
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
    info.level_number = get_little_endian_word(info_section.c_str());
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

    std::string sprite_data = input.get_section(MWLSection::sprite);
    info.sprite_header = sprite_data[0];
    // bit 6 of header specifies to use the new system, where FF is treated differently
    sprites = parse_sprite_data(sprite_data.substr(1), parse_extra_byte_table(extra_byte_counts), info.sprite_header & 0x20);

    std::string secondary_entrance_data = input.get_section(MWLSection::entrance);
    secondary_entrances = parse_secondary_entrances(secondary_entrance_data);

    // TODO: exanimation, gfx overrides
}
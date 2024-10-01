#pragma once

#include <cstdint>
#include <string>

#include <pcapplusplus/MacAddress.h>

// unsigned integers to big-endian byte strings
std::string uint_to_be_str(uint8_t i);
std::string uint_to_be_str(uint16_t i);
std::string uint_to_be_str(uint32_t i);
std::string mac_to_be_str(const pcpp::MacAddress &);

// big-endian byte strings to unsigned integers
uint8_t be_str_to_u8(const std::string &s);
uint16_t be_str_to_u16(const std::string &s);
uint32_t be_str_to_u32(const std::string &s);

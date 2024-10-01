#include "util.hpp"

#include <arpa/inet.h>
#include <cassert>

using namespace std;

string uint_to_be_str(uint8_t i) {
    return string(reinterpret_cast<char *>(&i), sizeof(i));
}

string uint_to_be_str(uint16_t i) {
    i = htons(i);
    return string(reinterpret_cast<char *>(&i), sizeof(i));
}

string uint_to_be_str(uint32_t i) {
    i = htonl(i);
    return string(reinterpret_cast<char *>(&i), sizeof(i));
}

string mac_to_be_str(const pcpp::MacAddress &mac) {
    return string(reinterpret_cast<const char *>(mac.getRawData()), 6);
}

uint8_t be_str_to_u8(const std::string &s) {
    auto p = reinterpret_cast<const uint8_t *>(s.c_str());
    return *p;
}

uint16_t be_str_to_u16(const std::string &s) {
    auto p = reinterpret_cast<const uint16_t *>(s.c_str());
    return ntohs(*p);
}

uint32_t be_str_to_u32(const std::string &s) {
    auto p = reinterpret_cast<const uint32_t *>(s.c_str());
    return ntohl(*p);
}

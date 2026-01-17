#pragma once
#include <string>
#include <cstddef>
#include <cstdint>

std::string base64_encode(const uint8_t* data, size_t len);

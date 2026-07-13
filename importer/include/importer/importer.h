/**
 * @file importer.h
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-07-09
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint>
#include <string>


// returns non-zero on success, otherwise 0.
uint32_t
import(std::string source_file, std::string target_dir);
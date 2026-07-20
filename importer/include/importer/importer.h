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

#include <cstdint>
#include <string>


struct allocator_t;

// returns non-zero on success, otherwise 0.
uint32_t
import(
    const std::string &source_file,
    const std::string &target_dir,
    const allocator_t *allocator);
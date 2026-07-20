/**
 * @file textures.h
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

uint32_t
import_texture(
  const std::string &source_file,
  const std::string &target_dir,
  const allocator_t *allocator);
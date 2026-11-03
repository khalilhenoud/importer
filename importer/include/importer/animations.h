/**
 * @file animations.h
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-10-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <string>


// will export all animations from a file.
void
import_animations(
  const std::string &source_file,
  const std::string &target_dir);
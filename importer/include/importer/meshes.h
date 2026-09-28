/**
 * @file meshes.h
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2023-12-21
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <string>


// will export all the meshes if true, otherwise the first one
void
import_meshes(
  const std::string &source_file,
  const std::string &target_dir,
  bool_t all = true);
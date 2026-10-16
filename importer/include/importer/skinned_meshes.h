/**
 * @file skinned_meshes.h
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


// will import both the skinned mesh and the skeleton separately, as well as the
// anim (optionally). The skeleton component should have the right skinned asset
// and skeleton referenced for proper behavior.
void
import_skinned_meshes(
  const std::string &source_file,
  const std::string &target_dir,
  bool import_anim = false);
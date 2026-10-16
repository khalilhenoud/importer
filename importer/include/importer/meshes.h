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


struct aiScene;

enum class mesh_import_type_t {
  mesh_non_skinned,
  mesh_skinned,
  mesh_all
};

void
import_meshes(
  const std::string &source_file,
  const std::string &target_dir);

void
import_meshes(
  const aiScene *pScene,
  const std::string &source_file,
  const std::string &target_dir,
  mesh_import_type_t types = mesh_import_type_t::mesh_non_skinned);
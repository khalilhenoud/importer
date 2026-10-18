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

#include <cstdint>
#include <string>


struct aiScene;

enum class mesh_import_type_t {
  mesh_non_skinned,
  mesh_skinned,
  mesh_all
};

// TODO: extend as needed
struct mesh_import_info_t {
  uint32_t vertex_count = 0;
  uint32_t index_count = 0;
};

mesh_import_info_t
import_mesh(
  const aiScene *pScene,
  const uint32_t index,
  const std::string &source_file,
  const std::string &target_dir);

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
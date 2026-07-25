/**
 * @file importer.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-07-09
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <importer/importer.h>
#include <importer/sublevels.h>
#include <importer/textures.h>
#include <importer/utils.h>


uint32_t
import(
  const std::string &source_file,
  const std::string &target_dir)
{
  assert(!source_file.empty());
  assert(!target_dir.empty());

  std::string extension = get_extension(source_file);
  if (extension == "png")
    return import_texture(source_file, target_dir);
  else if (extension == "map")
    return import_map(source_file, target_dir);

  return 0;
}
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
#include <importer/utils.h>
#include <importer/textures.h>


uint32_t
import(std::string source_file, std::string target_dir)
{
  assert(source_file && target_dir);

  std::string extension = get_extension(source_file);
  if (extension == "png")
    return import_texture(source_file, target_dir);

  return 0;
}
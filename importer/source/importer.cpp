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
#include <cassert>
#include <importer/importer.h>
#include <importer/fonts.h>
#include <importer/sublevels.h>
#include <importer/textures.h>
#include <importer/utils.h>


void
import(
  const std::string &source_file,
  const std::string &target_dir)
{
  assert(!source_file.empty());
  assert(!target_dir.empty());

  std::string extension = get_extension(source_file);
  if (extension == "png")
    import_texture(source_file, target_dir);
  else if (extension == "map")
    import_map(source_file, target_dir);
}

void
import(
  const std::string &source_file1,
  const std::string &source_file2,
  const std::string &target_dir)
{
  assert(!source_file1.empty());
  assert(!source_file2.empty());
  assert(!target_dir.empty());

  std::string extension1 = get_extension(source_file1);
  std::string extension2 = get_extension(source_file2);
  if (extension1 == "csv" && extension2 == "png")
    import_font(source_file1, source_file2, target_dir);
  assert(false);
}
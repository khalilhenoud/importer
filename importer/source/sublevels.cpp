/**
 * @file sublevels.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-07-23
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <cassert>
#include <string>
#include <unordered_map>
#include <importer/sublevels.h>
#include <importer/textures.h>
#include <importer/utils.h>
#include <level/sublevel_asset.h>
#include <library/allocator/allocator.h>
#include <library/filesystem/io.h>
#include <library/streams/binary_stream.h>
#include <library/type_registry/type_registry.h>
#include <loaders/loader_map.h>


struct texture_entry_t {
  std::string path;
  texture_info_t info;
};

using sanitized_path_t = std::string;
using texture_map_t = std::unordered_map<sanitized_path_t, texture_entry_t>;

// extracts the textures from the wad into a temporary folder. we then import
// the pngs, and delete the temporary folder.
void
extract_textures(
  const std::string &source_file,
  const std::string &target_dir,
  texture_map_t &texture_map,
  std::string wad_file_relative);

void
import_map(
  const std::string &source_file,
  const std::string &target_dir)
{
  loader_map_data_t *map = load_map(source_file.c_str(), &g_default_allocator);

  texture_map_t texture_map;
  extract_textures(source_file, target_dir, texture_map, map->world.wad);


  free_map(map, &g_default_allocator);

  return 1;
}

void
extract_textures(
  const std::string &source_file,
  const std::string &target_dir,
  texture_map_t &texture_map,
  std::string wad_file_relative)
{
  // TODO: remove ASAP, should be part of the working directory.
  static std::string tools_folder = "F:\\importer\\tools\\";

  // wad extraction directory
  std::string extraction_dir = source_file;
  auto iter = extraction_dir.find_last_of("\\/");
  extraction_dir = extraction_dir.substr(0, iter + 1);
  std::replace(wad_file_relative.begin(), wad_file_relative.end(), '/', '\\');
  extraction_dir += wad_file_relative;

  auto current_working_dir = std::filesystem::current_path();
  ensure_clean_directory(extraction_dir);
  std::filesystem::current_path(extraction_dir);

  {
    std::string wad_file = extraction_dir + ".wad";
    auto wad_canonical = std::filesystem::canonical(wad_file).string();

    // extract the wad into the directory
    std::string buffer = tools_folder;
    buffer += "qpakman -extract ";
    buffer += wad_canonical;
    system(buffer.c_str());
  }

  // restore working directory
  std::filesystem::current_path(current_working_dir);

  // remove '_fbr', qpakman appends '_fbr' to some textures after extraction.
  // in addition it automatically replaces os unsafe tokens with substrings
  // e.g: '*' becomes 'star_'.
  auto extracted_files = get_all_files(extraction_dir);
  for (const auto &file : extracted_files) {
    std::string path = get_simple_name(file.u8string());
    auto sanitized = path;
    string_utils::replace(sanitized, "_fbr", "");
    texture_info_t info = import_texture(file.u8string().c_str(), target_dir);
    texture_map[sanitized] = { path, info };
  }

  // cleanup
  assert(std::filesystem::remove_all(extraction_dir));

  return textures;
}
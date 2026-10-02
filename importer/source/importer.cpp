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
#include <cstring>
#include <importer/importer.h>
#include <importer/fonts.h>
#include <importer/meshes.h>
#include <importer/sublevels.h>
#include <importer/textures.h>
#include <importer/utils.h>
#include <library/cmdline/cmdline.h>


void
import(int argc, char *argv[])
{
  cmd_repo_t repo = {};
  uint32_t total = parse_cmdline_args(&repo, argc, argv);

  assert(repo.list.used > 1 && "use this with proper args");

  // [--texture|-t] source_file target_dir
  // [--map] source_file target_dir
  // [--font|-f] source_file_csv source_file_png target_dir
  // [--mesh|-m] source_file target_dir
  if (
    !strcmp(repo.list.entries[1].ptr, "--texture") ||
    !strcmp(repo.list.entries[1].ptr, "-t")) {
    assert(repo.list.used > 3);

    std::string source_file = repo.list.entries[2].ptr;
    assert(get_extension(source_file) == "png");

    std::string target_dir = repo.list.entries[3].ptr;
    import_texture(source_file, target_dir);
  } else if (!strcmp(repo.list.entries[1].ptr, "--map")) {
    assert(repo.list.used > 3);

    std::string source_file = repo.list.entries[2].ptr;
    assert(get_extension(source_file) == "map");

    std::string target_dir = repo.list.entries[3].ptr;
    import_map(source_file, target_dir);
  } else if (
    !strcmp(repo.list.entries[1].ptr, "--font") ||
    !strcmp(repo.list.entries[1].ptr, "-f")) {
    assert(repo.list.used > 4);

    std::string source_file1 = repo.list.entries[2].ptr;
    assert(get_extension(source_file1) == "csv");

    std::string source_file2 = repo.list.entries[3].ptr;
    assert(get_extension(source_file2) == "png");

    std::string target_dir = repo.list.entries[4].ptr;
    import_font(source_file1, source_file2, target_dir);
  } else if (
    !strcmp(repo.list.entries[1].ptr, "--mesh") ||
    !strcmp(repo.list.entries[1].ptr, "-m")) {
    assert(repo.list.used > 3);

    std::string source_file = repo.list.entries[2].ptr;
    std::string target_dir = repo.list.entries[3].ptr;
    import_meshes(source_file, target_dir);
  } else
    assert(false && "unsupported format!");
}
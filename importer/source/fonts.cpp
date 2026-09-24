/**
 * @file fonts.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-09-23
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <cassert>
#include <cstdint>
#include <font/font_asset.h>
#include <importer/fonts.h>
#include <importer/textures.h>
#include <importer/utils.h>
#include <library/allocator/allocator.h>
#include <library/streams/binary_stream.h>
#include <library/string/cstring.h>
#include <library/type_registry/type_registry.h>
#include <loaders/loader_csv.h>
#include <texture/texture_asset.h>


static
void
copy_font_data(const loader_csv_font_data_t *source, font_asset_t *target)
{
  target->texture_width = source->image_width;
  target->texture_height = source->image_height;
  target->cell_width = source->cell_width;
  target->cell_height = source->cell_height;
  target->font_width = source->font_width;
  target->font_height = source->font_height;
  target->start_char = source->start_char;

  for (uint32_t i = 0; i < FONT_GLYPH_COUNT; ++i) {
    memcpy(
      target->bounds[i],
      source->bounds[i].data,
      sizeof(source->bounds[i].data));
  }
}

void
import_font(
  const std::string &source_file1,
  const std::string &source_file2,
  const std::string &target_dir)
{
  loader_csv_font_data_t *font_csv = load_csv(
    source_file1.c_str(), &g_default_allocator);

  font_asset_t font = {};
  copy_font_data(font_csv, &font);
}
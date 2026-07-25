/**
 * @file textures.cpp
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
#include <importer/textures.h>
#include <importer/utils.h>
#include <library/allocator/allocator.h>
#include <library/filesystem/io.h>
#include <library/streams/binary_stream.h>
#include <library/type_registry/type_registry.h>
#include <loaders/loader_png.h>
#include <texture/texture_asset.h>


uint32_t
import_texture(
  const std::string &source_file,
  const std::string &target_dir)
{
  loader_png_data_t *data = load_png(source_file.c_str(), &g_default_allocator);
  assert(data);

  texture_asset_t asset;
  cvector_setup(
      &asset.buffer,
      get_type_data(uint8_t),
      data->total_buffer_size,
      &g_default_allocator);
  memcpy(asset.buffer.data, data->buffer, data->total_buffer_size);
  asset.format = (texture_format_t)data->format;
  asset.width = data->width;
  asset.height = data->height;

  // serialize to file
  binary_stream_t stream;
  binary_stream_def(&stream);
  binary_stream_setup(&stream, &g_default_allocator);
  texture_asset_serialize(&asset, &stream);

  std::string simple_name = get_simple_name(source_file);
  std::string type_dir = texture_asset_get_dir();
  std::string target_bin = target_dir + "\\" + type_dir;
  ensure_directory(target_bin);
  std::string target_file = target_bin + "\\" + simple_name + ".bin";
  write_to_file(stream, target_file);

  binary_stream_cleanup(&stream);
  free_png(data, &g_default_allocator);
  return 1;
}
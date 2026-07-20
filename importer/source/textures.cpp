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
#include <importer/textures.h>
#include <loaders/loader_png.h>
#include <library/allocator/allocator.h>


uint32_t
import_texture(
    const std::string &source_file,
    const std::string &target_dir,
    const allocator_t *allocator)
{
    loader_png_data_t *data = load_png(source_file.c_str(), allocator);
    return 1;
}
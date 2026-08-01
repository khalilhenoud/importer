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
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <importer/sublevels.h>
#include <importer/textures.h>
#include <importer/topology/poly_brush.h>
#include <importer/topology/polygon.h>
#include <importer/utils.h>
#include <level/sublevel_asset.h>
#include <library/allocator/allocator.h>
#include <library/streams/binary_stream.h>
#include <library/string/cstring.h>
#include <library/type_registry/type_registry.h>
#include <loaders/loader_map.h>
#include <material/bulk_material_asset.h>
#include <material/indexed_material_asset.h>
#include <material/material_asset.h>
#include <mesh/mesh_asset.h>
#include <texture/texture_asset.h>


struct texture_entry_t {
  std::string path;
  texture_info_t info;
};

using sanitized_path_t = std::string;
using texture_map_t = std::unordered_map<sanitized_path_t, texture_entry_t>;
using face_ids_t = std::vector<uint32_t>;
using texture_face_map_t = std::unordered_map<sanitized_path_t, face_ids_t>;
using topological_faces_t = std::vector<topology::face_t>;

static
void
extract_textures(
  const std::string &source_file,
  const std::string &target_dir,
  texture_map_t &texture_map,
  std::string wad_file_relative);

static
void
extract_faces(
  const loader_map_data_t *map,
  const texture_map_t &texture_map,
  topological_faces_t &topological_faces,
  texture_face_map_t &tface_map);

static
void
extract_materials(
  const texture_map_t &texture_map,
  const std::string &target_dir,
  const std::string &wad_file);

static
void
extract_meshes(
  const texture_map_t &texture_map,
  const topological_faces_t &topological_faces,
  const texture_face_map_t &tface_map);

void
import_map(
  const std::string &source_file,
  const std::string &target_dir)
{
  loader_map_data_t *map = load_map(source_file.c_str(), &g_default_allocator);
  std::string simple_wad_name = get_simple_name(map->world.wad);

  // extract textures
  texture_map_t texture_map;
  extract_textures(source_file, target_dir, texture_map, map->world.wad);
  extract_materials(texture_map, target_dir, simple_wad_name);

  topological_faces_t topological_faces;
  texture_face_map_t tface_map;
  extract_faces(map, texture_map, topological_faces, tface_map);

  // extract bulk meshes, where each mesh references an indexed_material_asset_t
  extract_meshes(texture_map, topological_faces, tface_map);

  free_map(map, &g_default_allocator);

  return 1;
}

static
void
setup_mesh(
  const std::pair<const sanitized_path_t, texture_entry_t> &entry,
  const topological_faces_t &topological_faces,
  const texture_face_map_t &tface_map,
  mesh_asset_t &mesh)
{
  auto &face_indices = tface_map[entry.first];
  uint32_t face_count = face_indices.size();
  uint32_t vertices_count = face_count * 3;
  cvector_setup(&mesh.vertices, get_type_data(float), 0, allocator);
  cvector_resize(&mesh.vertices, vertices_count * 3);
  cvector_setup(&mesh.normals, get_type_data(float), 0, allocator);
  cvector_resize(&mesh.normals, vertices_count * 3);
  cvector_setup(&mesh.uvs, get_type_data(float), 0, allocator);
  cvector_resize(&mesh.uvs, vertices_count * 3);
  memset(mesh.uvs.data, 0, sizeof(float) * vertices_count * 3);
  cvector_setup(&mesh.indices, get_type_data(uint32_t), 0, allocator);
  cvector_resize(&mesh.indices, vertices_count);

  // set the mesh specific data: vertices, uvs, indices, etc...
  uint32_t verti = 0, indexi = 0;
  uint32_t sizef3 = sizeof(float) * 3;
  float *vertices = (float *)mesh.vertices.data;
  float *normals = (float *)mesh.normals.data;
  float *uvs = (float *)mesh.uvs.data;
  uint32_t *indices = (uint32_t *)mesh.indices.data;
  for (uint32_t k = 0; k < face_count; ++k) {
    auto& face = topological_faces[face_indices[k]];
    point3f *points = face.face.points;
    memcpy(vertices + (verti + 0) * 3, points[0].data, sizef3);
    memcpy(vertices + (verti + 1) * 3, points[1].data, sizef3);
    memcpy(vertices + (verti + 2) * 3, points[2].data, sizef3);
    memcpy(normals + (verti + 0) * 3, face.normal.data, sizef3);
    memcpy(normals + (verti + 1) * 3, face.normal.data, sizef3);
    memcpy(normals + (verti + 2) * 3, face.normal.data, sizef3);
    memcpy(uvs + (verti + 0) * 3, face.uv[0].data, sizef3);
    memcpy(uvs + (verti + 1) * 3, face.uv[1].data, sizef3);
    memcpy(uvs + (verti + 2) * 3, face.uv[2].data, sizef3);

    indices[indexi + 0] = verti + 0;
    indices[indexi + 1] = verti + 1;
    indices[indexi + 2] = verti + 2;

    indexi += 3;
    verti += 3;
  }

  // 2- serialize an indexed_material_asset.h
  // 3- set the mesh material to point to the indexed_material_asset.h
}

void
extract_meshes(
  const texture_map_t &texture_map,
  const topological_faces_t &topological_faces,
  const texture_face_map_t &tface_map)
{
  bulk_mesh_asset_t asset = {};
  cvector_setup(
    &asset.meshes,
    get_type_data(mesh_asset_t),
    texture_map.size(),
    g_default_allocator);

  for (const auto &entry : texture_map) {
    mesh_asset_t mesh = {};
    setup_mesh(entry, topological_faces, tface_map, mesh);
    cvector_push_back(&asset.meshes, mesh, mesh_asset_t)
  }

  for (auto& entry : tex_map) {
    uint32_t i = entry.second.index;
    mesh_t *mesh = cvector_as(&scene->mesh_repo, i, mesh_t);
    mesh_def(mesh);

    // get the faces that share this index texture-material.
    auto& face_indices = entry.second.indices;
    uint32_t face_count = face_indices.size();
    uint32_t vertices_count = face_count * 3;
    uint32_t sizef3 = sizeof(float) * 3;

    cvector_setup(&mesh->vertices, get_type_data(float), 0, allocator);
    cvector_resize(&mesh->vertices, vertices_count * 3);
    cvector_setup(&mesh->normals, get_type_data(float), 0, allocator);
    cvector_resize(&mesh->normals, vertices_count * 3);
    cvector_setup(&mesh->uvs, get_type_data(float), 0, allocator);
    cvector_resize(&mesh->uvs, vertices_count * 3);
    memset(mesh->uvs.data, 0, sizeof(float) * vertices_count * 3);
    cvector_setup(&mesh->indices, get_type_data(uint32_t), 0, allocator);
    cvector_resize(&mesh->indices, vertices_count);
    mesh->materials.used = 1;
    mesh->materials.indices[0] = i;

    // copy the data into the mesh.
    uint32_t verti = 0, indexi = 0;
    float *vertices = (float *)mesh->vertices.data;
    float *normals = (float *)mesh->normals.data;
    float *uvs = (float *)mesh->uvs.data;
    uint32_t *indices = (uint32_t *)mesh->indices.data;
    for (uint32_t k = 0; k < face_count; ++k) {
      auto& face = map_faces[face_indices[k]];
      point3f* points = face.face.points;
      memcpy(vertices + (verti + 0) * 3, points[0].data, sizef3);
      memcpy(vertices + (verti + 1) * 3, points[1].data, sizef3);
      memcpy(vertices + (verti + 2) * 3, points[2].data, sizef3);
      memcpy(normals + (verti + 0) * 3, face.normal.data, sizef3);
      memcpy(normals + (verti + 1) * 3, face.normal.data, sizef3);
      memcpy(normals + (verti + 2) * 3, face.normal.data, sizef3);
      memcpy(uvs + (verti + 0) * 3, face.uv[0].data, sizef3);
      memcpy(uvs + (verti + 1) * 3, face.uv[1].data, sizef3);
      memcpy(uvs + (verti + 2) * 3, face.uv[2].data, sizef3);

      indices[indexi + 0] = verti + 0;
      indices[indexi + 1] = verti + 1;
      indices[indexi + 2] = verti + 2;

      indexi += 3;
      verti += 3;
    }
  }
}

static
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
}

static
void
set_mat_rgb(mat_rgba_t &color, float r, float g, float b, float a = 1.f)
{
  color.data[0] = r;
  color.data[1] = g;
  color.data[2] = b;
  color.data[3] = a;
}

static
void
setup_material_asset(
  material_asset_t &material,
  const std::string &target_dir,
  const std::string &name)
{
  material_asset_def(&material);
  cstring_setup2(&material.name, name.c_str());
  material.opacity = material.shininess = 1.f;
  set_mat_rgb(material.ambient, 0.5f, 0.5f, 0.5f);
  set_mat_rgb(material.diffuse, 0.5f, 0.5f, 0.5f);
  set_mat_rgb(material.specular, 0.5f, 0.5f, 0.5f);

  cvector_setup2(&material.textures, get_type_data(texture_properties_t));
  texture_properties_t texture = {};
  texture.texture_ref.type_id = get_type_id(texture_asset_t);
  cstring_setup2(&texture.texture_ref.path, name.c_str());
  cvector_push_back(&material.textures, texture, texture_properties_t);
}

void
extract_materials(
  const texture_map_t &texture_map,
  const std::string &target_dir,
  const std::string &wad_file)
{
  bulk_material_asset_t asset = {};

  cvector_setup2(&asset.materials, get_type_data(material_asset_t));
  for (const auto &entry : texture_map) {
    material_asset_t material;
    setup_material_asset(material, target_dir, entry.second.path);
    cvector_push_back(&asset, &material, material_asset_t);
  }

  binary_stream_t stream;
  binary_stream_def(&stream);
  binary_stream_setup(&stream, &g_default_allocator);
  bulk_material_asset_serialize(&asset, &stream);

  std::string type_dir = bulk_material_asset_get_dir();
  std::string target_bin = target_dir + "\\" + type_dir;
  ensure_directory(target_bin);
  std::string target_file = target_bin + "\\" + wad_file + ".bin";
  write_to_file(stream, target_file);

  binary_stream_cleanup(&stream);
  bulk_material_asset_cleanup(&asset, &g_default_allocator);
}

static
void
extract_faces(
  const loader_map_data_t *map,
  const texture_map_t &texture_map,
  topological_faces_t &topological_faces,
  texture_face_map_t &tface_map)
{
  // convert to the format the brush expect
  std::unordered_map<std::string, topology::texture_info_t> textures_info;
  for (const auto &entry : texture_map)
    textures_info[entry.first] = {
      entry.second.info.width, entry.second.info.height };

  std::vector<topology::poly_brush_t> poly_brushes;
  for (uint32_t i = 0; i < map_data->world.brush_count; ++i) {
    const topology::brush_t brush(map_data->world.brushes + i, textures_info);
    poly_brushes.emplace_back(&brush);
  }

  topology::poly_brush_t::sort_and_weld(poly_brushes);

  for (uint32_t i = 0; i < poly_brushes.size(); ++i) {
    const topology::poly_brush_t &poly_brush = poly_brushes[i];
    std::vector<topology::face_t> faces = poly_brush.to_faces();

    for (uint32_t j = 0; j < faces.size(); ++j) {
      auto& face = faces[j];
      if (face.texture.size())
        tface_map[face.texture].push_back(topological_faces.size() + j);
    }

    topological_faces.insert(
      topological_faces.end(), faces.begin(), faces.end());
  }
}
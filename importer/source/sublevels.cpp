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
#include <props/light.h>
#include <spatial/bvh/bvh.h>
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
  bulk_mesh_asset_t &asset,
  const texture_map_t &texture_map,
  const std::string &target_dir,
  const topological_faces_t &topological_faces,
  const texture_face_map_t &tface_map,
  const std::string &wad_name);

static
void
extract_lights(
  cvector_t &lights,
  loader_map_data_t &map,
  const matrix4f &transform);

static
void
build_bvh(
  bvh_t &bvh,
  bulk_mesh_asset_t &meshes,
  const matrix4f &transform);

void
import_map(
  const std::string &source_file,
  const std::string &target_dir)
{
  loader_map_data_t *map = load_map(source_file.c_str(), &g_default_allocator);
  std::string wad_name = get_simple_name(map->world.wad);
  std::string map_name = get_simple_name(source_file);

  sublevel_asset_t sublevel = {};
  cstring_setup2(&sublevel.name, map_name.c_str());
  // set the default transform
  matrix4f_rotation_x(&sublevel.transform, -K_PI/2.f);

  // copy the meta data
  sublevel.metadata.player_start.data[0] = (float)map->player_start[0];
  sublevel.metadata.player_start.data[1] = (float)map->player_start[1];
  sublevel.metadata.player_start.data[2] = (float)map->player_start[2];
  sublevel.metadata.player_angle = (float)map->player_angle;

  // extract textures
  texture_map_t texture_map;
  extract_textures(source_file, target_dir, texture_map, map->world.wad);
  extract_materials(texture_map, target_dir, wad_name);

  topological_faces_t topological_faces;
  texture_face_map_t tface_map;
  extract_faces(map, texture_map, topological_faces, tface_map);

  // each mesh references an indexed_material_asset_t that is exported
  extract_meshes(
    sublevel.meshes,
    texture_map, target_dir, topological_faces, tface_map, wad_name);

  extract_lights(sublevel.lights, *map, sublevel.transform);

  build_bvh(sublevel.bvh, sublevel.meshes, sublevel.transform);

  free_map(map, &g_default_allocator);

  write_to_file(
    target_dir,
    &sublevel,
    sublevel_asset_serialize, sublevel_asset_get_dir,
    map_name, "bin");

  sublevel_asset_cleanup(&sublevel, &g_default_allocator);
}

static
void
build_bvh_transformed_vertices(
  const mesh_asset_t &mesh,
  const uint32_t index,
  const matrix4f &transform,
  float **vertices,
  uint32_t **indices,
  uint32_t *indices_count)
{
  // transform the original vertices by the sublevel transform
  float *points = (float*)g_default_allocator.mem_cont_alloc(
    mesh.vertices.size, sizeof(float));
  memcpy(points, mesh.vertices.data, mesh.vertices.size * sizeof(float));

  for (uint32_t i = 0, count = (mesh.vertices.size / 3); i < count; ++i) {
    point3f pt = { points[i * 3 + 0], points[i * 3 + 1], points[i * 3 + 2] };
    mult_set_m4f_p3f(&transform, &pt);
    memcpy(&points[i * 3 + 0], pt.data, sizeof(pt.data));
  }

  vertices[index] = points;
  indices[index] = (uint32_t *)mesh.indices.data;
  indices_count[index] = mesh.indices.size;
}

static
void
build_bvh(
  bvh_t &level_bvh,
  bulk_mesh_asset_t &bulk_mesh,
  const matrix4f &transform)
{
  if (!bulk_mesh.meshes.size)
    return;

  bvh_t *bvh = NULL;
  float **vertices = NULL;
  uint32_t **indices = NULL;
  uint32_t *indices_count = NULL;
  uint32_t mesh_count = bulk_mesh.meshes.size;

  vertices = (float **)g_default_allocator.mem_alloc(
    sizeof(float *) * mesh_count);
  indices = (uint32_t **)g_default_allocator.mem_alloc(
    sizeof(uint32_t *) * mesh_count);
  indices_count = (uint32_t *)g_default_allocator.mem_alloc(
    sizeof(uint32_t) * mesh_count);
  assert(vertices && indices && indices_count);

  for (uint32_t i = 0; i < bulk_mesh.meshes.size; ++i) {
    mesh_asset_t *mesh = cvector_as(&bulk_mesh.meshes, i, mesh_asset_t);
    build_bvh_transformed_vertices(
      *mesh, i, transform, vertices, indices, indices_count);
  }

  bvh = bvh_create(
    vertices,
    indices,
    indices_count,
    mesh_count,
    &g_default_allocator,
    BVH_CONSTRUCT_NAIVE);

  for (uint32_t i = 0; i < mesh_count; ++i)
    g_default_allocator.mem_free(vertices[i]);
  g_default_allocator.mem_free(vertices);
  g_default_allocator.mem_free(indices);
  g_default_allocator.mem_free(indices_count);

  cvector_fullswap(&bvh->normals, &level_bvh.normals);
  cvector_fullswap(&bvh->faces, &level_bvh.faces);
  cvector_fullswap(&bvh->bounds, &level_bvh.bounds);
  cvector_fullswap(&bvh->nodes, &level_bvh.nodes);
  g_default_allocator.mem_free(bvh);
}

static
void
setup_light(
  light_t &light,
  loader_map_light_data_t &source,
  const matrix4f &transform)
{
  // set the position
  light.position.data[0] = (float)source.origin[0];
  light.position.data[1] = (float)source.origin[1];
  light.position.data[2] = (float)source.origin[2];
  mult_set_m4f_p3f(
    &transform,
    &light.position);

  light.type = LIGHT_TYPE_POINT;
  light.attenuation_constant = 1.f;
  light.attenuation_linear = 0.01f;
  light.attenuation_quadratic = 0.f;
  light.ambient.data[0] =
  light.ambient.data[1] =
  light.ambient.data[2] = 0.2f;
  light.ambient.data[3] = 1.f;
  light.diffuse.data[0] =
  light.diffuse.data[1] =
  light.diffuse.data[2] = (float)(source.light)/255.f;
  light.diffuse.data[3] = 1.f;
  light.specular.data[0] =
  light.specular.data[1] =
  light.specular.data[2] = 0.f;
  light.specular.data[3] = 1.f;
}

static
void
extract_lights(
  cvector_t &lights,
  loader_map_data_t &map,
  const matrix4f &transform)
{
  cvector_setup(&lights,
    get_type_data(light_t),
    map.lights.count,
    &g_default_allocator);
  cvector_resize(&lights, map.lights.count);

  std::string light_name;
  for (uint32_t i = 0; i < lights.size; ++i) {
    light_t *light = cvector_as(&lights, i, light_t);
    light_def(light);
    light_name = "light_" + std::to_string(i);
    cstring_setup2(&light->name, light_name.c_str());
    setup_light(*light, map.lights.lights[i], transform);
  }
}

static
void
setup_mesh(
  const std::string &target_dir,
  const std::string &wad_file,
  const uint32_t index,
  const sanitized_path_t sanitized,
  const topological_faces_t &topological_faces,
  const texture_face_map_t &tface_map,
  mesh_asset_t &mesh)
{
  const face_ids_t &face_indices = tface_map.at(sanitized);
  uint32_t face_count = face_indices.size();
  uint32_t vertices_count = face_count * 3;
  cvector_setup(&mesh.vertices, get_type_data(float), 0, &g_default_allocator);
  cvector_resize(&mesh.vertices, vertices_count * 3);
  cvector_setup(&mesh.normals, get_type_data(float), 0, &g_default_allocator);
  cvector_resize(&mesh.normals, vertices_count * 3);
  cvector_setup(&mesh.uvs, get_type_data(float), 0, &g_default_allocator);
  cvector_resize(&mesh.uvs, vertices_count * 3);
  memset(mesh.uvs.data, 0, sizeof(float) * vertices_count * 3);
  cvector_setup(
    &mesh.indices, get_type_data(uint32_t), 0, &g_default_allocator);
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
    const point3f *points = face.face.points;
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

  // serialize an indexed_material_asset_t, the name would be "suffixed?".
  std::string indexed_name = wad_file + "_" + std::to_string(index);
  indexed_material_asset_t imaterial;
  imaterial.bulk_material_ref.type_id = get_type_id(bulk_material_asset_t);
  cstring_setup2(&imaterial.bulk_material_ref.path, wad_file.c_str());
  imaterial.index = index;

  write_to_file(
    target_dir,
    &imaterial,
    indexed_material_asset_serialize,
    indexed_material_asset_get_dir,
    indexed_name, "bin");

  // set the mesh material to point to the indexed_material_asset_t
  cvector_setup2(&mesh.materials, asset_ref_t);
  asset_ref_t indexed_material = {};
  indexed_material.type_id = get_type_id(indexed_material_asset_t);
  cstring_setup2(&indexed_material.path, indexed_name.c_str());
  cvector_push_back(&mesh.materials, indexed_material, asset_ref_t);
}

static
void
extract_meshes(
  bulk_mesh_asset_t &asset,
  const texture_map_t &texture_map,
  const std::string &target_dir,
  const topological_faces_t &topological_faces,
  const texture_face_map_t &tface_map,
  const std::string &wad_name)
{
  cvector_setup(
    &asset.meshes,
    get_type_data(mesh_asset_t),
    texture_map.size(),
    &g_default_allocator);

  uint32_t index = 0;
  for (const auto &entry : texture_map) {
    if (tface_map.find(entry.first) == tface_map.cend()) {
      ++index;
      continue;
    }

    mesh_asset_t mesh = {};
    setup_mesh(
      target_dir, wad_name, index++,
      entry.first, topological_faces, tface_map, mesh);

    // TODO(@khalil): check if this continue happening.
    if (mesh.indices.size)
      cvector_push_back(&asset.meshes, mesh, mesh_asset_t);
    else
      assert(false);
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
    replace(sanitized, "_fbr", "");
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

  cvector_setup2(&material.textures, texture_properties_t);
  texture_properties_t texture = {};
  texture.texture_ref.type_id = get_type_id(texture_asset_t);
  cstring_setup2(&texture.texture_ref.path, name.c_str());
  cvector_push_back(&material.textures, texture, texture_properties_t);
}

static
void
extract_materials(
  const texture_map_t &texture_map,
  const std::string &target_dir,
  const std::string &wad_file)
{
  bulk_material_asset_t asset = {};

  cvector_setup2(&asset.materials, material_asset_t);
  for (const auto &entry : texture_map) {
    material_asset_t material;
    setup_material_asset(material, target_dir, entry.second.path);
    cvector_push_back(&asset.materials, material, material_asset_t);
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
  for (uint32_t i = 0; i < map->world.brush_count; ++i) {
    const topology::brush_t brush(map->world.brushes + i, textures_info);
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
/**
 * @file meshes.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2023-12-21
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <cassert>
#include <cstdint>
#include <iostream>
#include <importer/meshes.h>
#include <importer/textures.h>
#include <importer/utils.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <library/allocator/allocator.h>
#include <library/streams/binary_stream.h>
#include <library/string/cstring.h>
#include <library/containers/cvector.h>
#include <library/type_registry/type_registry.h>
#include <material/material_asset.h>
#include <mesh/mesh_asset.h>
#include <texture/texture_asset.h>


inline
uint32_t
count_material_textures(const aiMaterial *pMaterial)
{
  uint32_t total = 0;
  aiReturn value;
  uint32_t t_type_index = 1;
  uint32_t t_type_total = aiTextureType::AI_TEXTURE_TYPE_MAX;
  for (; t_type_index < t_type_total; ++t_type_index) {
    uint32_t texture_total = aiGetMaterialTextureCount(
      pMaterial,
      (aiTextureType)t_type_index);

    for (uint32_t i = 0; i < texture_total; ++i) {
      aiString path;
      value = aiGetMaterialTexture(
        pMaterial, (aiTextureType)t_type_index, i, &path);

      if (value == AI_SUCCESS)
        ++total;
    }
  }

  return total;
}

static
void
extract_material(
  const aiMaterial *pMaterial,
  const std::string &target_dir,
  std::string &file_name)
{
  material_asset_t material = {};

  aiColor4D color;
  aiReturn value;
  value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_DIFFUSE, &color);
  copy_color(material.diffuse.data, &color, value);
  value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_AMBIENT, &color);
  copy_color(material.ambient.data, &color, value);
  value = aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_SPECULAR, &color);
  copy_color(material.specular.data, &color, value);

  float data_float = 1.f;
  value = aiGetMaterialFloat(pMaterial, AI_MATKEY_OPACITY, &data_float);
  copy_float(&material.opacity, &data_float, value);
  value = aiGetMaterialFloat(pMaterial, AI_MATKEY_SHININESS, &data_float);
  copy_float(&material.shininess, &data_float, value);

  // also use this as the material file name
  aiString data_str;
  value = aiGetMaterialString(pMaterial, AI_MATKEY_NAME, &data_str);
  assert(value == AI_SUCCESS);
  cstring_setup2(&material.name, data_str.C_Str());

  uint32_t textures_count = count_material_textures(pMaterial);
  cvector_setup2(&material.textures, texture_properties_t);
  cvector_resize(&material.textures, textures_count);

  uint32_t index = 0;
  uint32_t t_type_index = 1;
  uint32_t t_type_total = aiTextureType::AI_TEXTURE_TYPE_MAX;
  for (; t_type_index < t_type_total; ++t_type_index) {
    uint32_t texture_total = aiGetMaterialTextureCount(
      pMaterial,
      (aiTextureType)t_type_index);

    for (uint32_t i = 0; i < texture_total; ++i) {
      aiString path;
      value = aiGetMaterialTexture(
        pMaterial, (aiTextureType)t_type_index, i, &path);

      if (value == AI_SUCCESS) {
        import_texture(path.C_Str(), target_dir);

        texture_properties_t *properties = cvector_as(
          &material.textures, index, texture_properties_t);
        ++index;

        properties->texture_ref.type_id = get_type_id(texture_asset_t);
        std::string name = get_simple_name(path.C_Str());
        std::string path = construct_asset_path(
          target_dir, texture_asset_get_dir, name);
        cstring_setup2(&properties->texture_ref.path, path.c_str());

        aiUVTransform transform;
        value = aiGetMaterialUVTransform(
          pMaterial,
          AI_MATKEY_UVTRANSFORM((aiTextureType)t_type_index, i),
          &transform);
        copy_texture_transform(properties, &transform, value);
      }
    }
  }

  // save the material to disk
  binary_stream_t stream;
  binary_stream_def(&stream);
  binary_stream_setup(&stream, &g_default_allocator);
  material_asset_serialize(&material, &stream);

  std::string type_dir = material_asset_get_dir();
  std::string target_bin = target_dir + "\\" + type_dir;
  ensure_directory(target_bin);
  file_name = get_simple_name(data_str.C_Str());
  std::string target_file = target_bin + "\\" + file_name + ".bin";
  write_to_file(stream, target_file);

  binary_stream_cleanup(&stream);
  material_asset_cleanup(&material, &g_default_allocator);
}

void
copy_mesh_topology(
  mesh_asset_t *mesh,
  const aiMesh *pMesh)
{
  uint32_t face_count = pMesh->mNumFaces;
  cvector_setup2(&mesh->indices, uint32_t);
  cvector_resize(&mesh->indices, face_count * 3);
  for (uint32_t i = 0; i < face_count; ++i) {
    aiFace *face = &pMesh->mFaces[i];
    assert(face->mNumIndices == 3 && "We do not support non-triangle meshes!!");
    uint32_t *indices = ((uint32_t *)mesh->indices.data) + i * 3;
    indices[0] = face->mIndices[0];
    indices[1] = face->mIndices[1];
    indices[2] = face->mIndices[2];
  }

  uint32_t vertices_count = pMesh->mNumVertices;
  cvector_setup2(&mesh->vertices, float);
  cvector_resize(&mesh->vertices, vertices_count * 3);
  memset(mesh->vertices.data, 0, sizeof(float) * vertices_count * 3);
  assert(pMesh->mVertices && "mesh with null vertex buffer!");
  for (uint32_t i = 0; i < vertices_count; ++i) {
    aiVector3D *pVertex = &pMesh->mVertices[i];
    float *vertex = ((float *)mesh->vertices.data) + i * 3;
    vertex[0] = pVertex->x;
    vertex[1] = pVertex->y;
    vertex[2] = pVertex->z;
  }

  cvector_setup2(&mesh->normals, float);
  cvector_resize(&mesh->normals, vertices_count * 3);
  assert(pMesh->mNormals && "mesh with null normal buffer!");
  for (uint32_t i = 0; i < vertices_count; ++i) {
    aiVector3D *pNormal = &pMesh->mNormals[i];
    float *normal = ((float *)mesh->normals.data) + i * 3;
    normal[0] = pNormal->x;
    normal[1] = pNormal->y;
    normal[2] = pNormal->z;
  }

  // NOTE: assimp supports 8 channels for vertices, we only consider the first.
  // TODO: consider supporting more than 1 channel.
  if (pMesh->mTextureCoords[0]) {
    cvector_setup2(&mesh->uvs, float);
    cvector_resize(&mesh->uvs, vertices_count * 3);

    for (uint32_t i = 0; i < vertices_count; ++i) {
      aiVector3D *pUVs = &pMesh->mTextureCoords[0][i];
      float *uvs = ((float *)mesh->uvs.data) + i * 3;
      uvs[0] = pUVs->x;
      uvs[1] = pUVs->y;
      uvs[2] = pUVs->z;
    }
  }
}

void
import_meshes(
  const aiScene *pScene,
  const std::string &source_file,
  const std::string &target_dir)
{
  // NOTE: Currently assimp will decompose the mesh if it contains more than
  // one material, so basically a single material is specified. The rest of
  // the materials can be found on identically named meshes.
  // Additionally no transform is assigned to the mesh, instead it uses the
  // transform attached to the parent node.
  // TODO(@khalil): investigate multi-material import.
  std::string file_name;
  for (uint32_t i = 0, scene_i = 0; i < pScene->mNumMeshes; ++i) {
    // skip skeletal meshes
    if (pScene->mMeshes[i]->HasBones())
      continue;

    aiMesh *pMesh = pScene->mMeshes[i];
    mesh_asset_t mesh = {};
    // assimp garantees at least one material, unless incomplete flag is set
    extract_material(
      pScene->mMaterials[pMesh->mMaterialIndex], target_dir, file_name);

    copy_mesh_topology(&mesh, pMesh);

    // set the mesh material
    cvector_setup2(&mesh.materials, asset_ref_t);
    cvector_resize(&mesh.materials, 1);
    asset_ref_t *material_ref = cvector_as(&mesh.materials, 0, asset_ref_t);
    material_ref->type_id = get_type_id(material_asset_t);
    std::string path = construct_asset_path(
      target_dir, material_asset_get_dir, file_name);
    cstring_setup2(&material_ref->path, path.c_str());

    binary_stream_t stream;
    binary_stream_def(&stream);
    binary_stream_setup(&stream, &g_default_allocator);
    mesh_asset_serialize(&mesh, &stream);

    std::string type_dir = mesh_asset_get_dir();
    std::string target_bin = target_dir + "\\" + type_dir;
    ensure_directory(target_bin);
    std::string mesh_name = get_simple_name(source_file);
    mesh_name += "_";
    mesh_name += pMesh->mName.C_Str();
    mesh_name += "_" + std::to_string(i);
    std::string target_file = target_bin + "\\" + mesh_name + ".bin";
    write_to_file(stream, target_file);

    binary_stream_cleanup(&stream);
    mesh_asset_cleanup(&mesh, &g_default_allocator);
  }
}

void
import_meshes(
  const std::string &source_file,
  const std::string &target_dir)
{
  Assimp::Importer Importer;
  // TODO(@khalil): need to reinvestigate this, is this still needed for anims?
  Importer.SetPropertyBool(AI_CONFIG_IMPORT_COLLADA_IGNORE_UNIT_SIZE, true);
  const aiScene *pScene = Importer.ReadFile(
    source_file.c_str(),
    aiProcess_Triangulate |
    aiProcess_GenSmoothNormals |
    aiProcess_FlipUVs |
    aiProcess_JoinIdenticalVertices);

  if (!pScene)
    std::cout << "Error parsing '" << source_file << "'" <<
    Importer.GetErrorString() << std::endl;
  else
    import_meshes(pScene, source_file, target_dir);
}
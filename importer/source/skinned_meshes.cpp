/**
 * @file skinned_meshes.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2026-10-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <cassert>
#include <cstdint>
#include <iostream>
#include <importer/skinned_meshes.h>
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
#include <mesh/skinned_mesh_asset.h>
#include <texture/texture_asset.h>


static
void
copy_bone_data(
  skinned_mesh_asset_t *mesh,
  aiMesh *pMesh,
  const aiScene *pScene)
{
  cvector_setup2(&mesh->bones, skinned_mesh_bone_t);
  cvector_resize(&mesh->bones, pMesh->mNumBones);

  for (uint32_t i = 0; i < pMesh->mNumBones; ++i) {
    aiBone *source = pMesh->mBones[i];
    skinned_mesh_bone_t *target = cvector_as(
      &mesh->bones, i, skinned_mesh_bone_t);

    cstring_setup2(&target->name, source->mName.C_Str());

    // copy the offset matrix
    ::matrix4f_set_identity(&target->offset_matrix);
    aiMatrix4x4& transform = source->mOffsetMatrix;
    float data[16] = {
      transform.a1, transform.a2, transform.a3, transform.a4,
      transform.b1, transform.b2, transform.b3, transform.b4,
      transform.c1, transform.c2, transform.c3, transform.c4,
      transform.d1, transform.d2, transform.d3, transform.d4};
    memcpy(target->offset_matrix.data, data, sizeof(float) * 16);

    cvector_setup(
      &target->vertex_weights, get_type_data(vertex_weight_t), 0, allocator);
    cvector_resize(&target->vertex_weights, source->mNumWeights);

    for (uint32_t j = 0; j < source->mNumWeights; ++j) {
      aiVertexWeight copy_from = source->mWeights[j];
      vertex_weight_t *copy_into = cvector_as(
        &target->vertex_weights, j, vertex_weight_t);
      copy_into->vertex_id = copy_from.mVertexId;
      copy_into->weight = copy_from.mWeight;
    }
  }
}

static
void
import_skinned_meshes(
  const aiScene *pScene,
  const std::string &source_file,
  const std::string &target_dir,
  bool import_anim)
{
  import_meshes(
    pScene, source_file, target_dir, mesh_import_type_t::mesh_skinned);

  for (uint32_t i = 0, scene_i = 0; i < pScene->mNumMeshes; ++i) {
    aiMesh *pMesh = pScene->mMeshes[i];

    if (!pMesh->HasBones())
      continue;

    // 5- optionally import the animations (separate files).

    skinned_mesh_asset_t mesh = {};
    mesh.mesh.type_id = get_type_id(mesh_asset_t);
    std::string path = construct_asset_path(
      target_dir, mesh_asset_get_dir, pMesh->mName.C_Str());
    cstring_setup2(&mesh.mesh.path, path.c_str());

    copy_bone_data(&mesh, pMesh, pScene);
    // import the skeleton, we are going to neeed this for the next function
    // 3- populate the skinned mesh data (the bones depend on the skeleton)
      // import the skeleon, there is no way around this, we need the skeleton
      // asset to properly import the data.

    binary_stream_t stream;
    binary_stream_def(&stream);
    binary_stream_setup(&stream, &g_default_allocator);
    skinned_mesh_asset_serialize(&mesh, &stream);

    std::string type_dir = skinned_mesh_asset_get_dir();
    std::string target_bin = target_dir + "\\" + type_dir;
    ensure_directory(target_bin);
    std::string mesh_name = pMesh->mName.C_Str();
    std::string target_file = target_bin + "\\" + mesh_name + ".bin";
    write_to_file(stream, target_file);

    binary_stream_cleanup(&stream);
    skinned_mesh_asset_cleanup(&mesh, &g_default_allocator);
  }
}

void
import_skinned_meshes(
  const std::string &source_file,
  const std::string &target_dir,
  bool import_anim)
{
  Assimp::Importer Importer;
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
    import_skinned_meshes(pScene, source_file, target_dir);
}
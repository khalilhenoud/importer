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
#include <functional>
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
#include <mesh/mesh_asset.h>
#include <mesh/skinned_mesh_asset.h>
#include <skeleton/skeleton_asset.h>
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
      &target->vertex_weights,
      get_type_data(bone_vertex_weight_t), 0, &g_default_allocator);
    cvector_resize(&target->vertex_weights, source->mNumWeights);

    for (uint32_t j = 0; j < source->mNumWeights; ++j) {
      aiVertexWeight copy_from = source->mWeights[j];
      bone_vertex_weight_t *copy_into = cvector_as(
        &target->vertex_weights, j, bone_vertex_weight_t);
      copy_into->vertex_id = copy_from.mVertexId;
      copy_into->weight = copy_from.mWeight;
    }
  }
}


static
void
read_skeleton(
  skeleton_asset_t *skeleton,
  const skinned_mesh_asset_t *skinned_mesh,
  aiMesh *pMesh,
  const aiScene *pScene)
{
  std::function<uint32_t(aiNode*)>
  get_bone_index = [&](aiNode *target) {
    for (uint32_t i = 0; i < skinned_mesh->bones.size; ++i) {
      const skinned_mesh_bone_t *bone =
        (const skinned_mesh_bone_t *)cvector_at_c(&skinned_mesh->bones, i);
      if (!strcmp(bone->name.str, target->mName.C_Str()))
        return i;
    }

    return std::numeric_limits<uint32_t>::max();
  };

  std::function<bool(aiNode*)>
  is_bone = [&](aiNode *target) {
    uint32_t index = get_bone_index(target);
    if (index != std::numeric_limits<uint32_t>::max())
      return true;
    return false;
  };

  std::function<aiNode*(aiNode *)>
  get_starting_bone = [&](aiNode *target) {
    if (is_bone(target))
      return target;

    for (uint32_t i = 0; i < target->mNumChildren; ++i)
      if (aiNode *result = get_starting_bone(target->mChildren[i]))
        return result;

    return (aiNode *)nullptr;
  };

  std::function<void(aiNode *, uint32_t&)>
  count_skel_bone_count = [&](aiNode *target, uint32_t& count) {
    for (uint32_t i = 0; i < target->mNumChildren; ++i)
      count_skel_bone_count(target->mChildren[i], ++count);
  };

  std::function<void(aiNode *, uint32_t&)>
  populate_bone_structure = [&](aiNode *source, uint32_t& index) {
    skeleton_node_t *target = cvector_as(
      &skeleton->nodes, index, skeleton_node_t);

    ::matrix4f_set_identity(&target->transform);
    aiMatrix4x4& transform = source->mTransformation;
    float data[16] = {
      transform.a1, transform.a2, transform.a3, transform.a4,
      transform.b1, transform.b2, transform.b3, transform.b4,
      transform.c1, transform.c2, transform.c3, transform.c4,
      transform.d1, transform.d2, transform.d3, transform.d4};
    memcpy(target->transform.data, data, sizeof(float) * 16);

    target->bone_index = get_bone_index(source);

    cstring_setup(&target->name, source->mName.C_Str(), &g_default_allocator);

    cvector_setup(
      &target->children, get_type_data(uint32_t), 0, &g_default_allocator);
    cvector_resize(&target->children, source->mNumChildren);
    for (uint32_t i = 0; i < source->mNumChildren; ++i) {
      uint32_t *indices = cvector_as(&target->children, i, uint32_t);
      *indices = ++index;
      populate_bone_structure(source->mChildren[i], index);
    }
  };

  aiNode *skel_root = get_starting_bone(pScene->mRootNode);
  assert(skel_root);
  uint32_t count = 1;
  count_skel_bone_count(skel_root, count);
  cvector_setup2(&skeleton->nodes, skeleton_node_t);
  cvector_resize(&skeleton->nodes, count);
  uint32_t index = 0;
  populate_bone_structure(skel_root, index);
}

static
void
adjust_child_bind_pose_transform(
  skinned_mesh_asset_t *skinned_mesh,
  skeleton_asset_t *skeleton,
  skeleton_node_t *node,
  matrix4f parent_transform)
{
  cvector_t *nodes = &skeleton->nodes;

  if (node->bone_index != UINT32_MAX) {
    skinned_mesh_bone_t *bone = cvector_as(
      &skinned_mesh->bones, node->bone_index, skinned_mesh_bone_t);
    matrix4f to_bind_pose = inverse_m4f(&bone->offset_matrix);
    node->transform = mult_m4f(&parent_transform, &to_bind_pose);
    for (uint32_t i = 0; i < node->children.size; ++i) {
      uint32_t index = *cvector_as(&node->children, i, uint32_t);
      skeleton_node_t *child = cvector_as(nodes, index, skeleton_node_t);
      adjust_child_bind_pose_transform(
        skinned_mesh, skeleton, child, bone->offset_matrix);
    }
  }
}

static
void
adjust_skinned_mesh_node_bind_pose_transform(
  skinned_mesh_asset_t *skinned_mesh,
  skeleton_asset_t *skeleton)
{
  cvector_t *nodes = &skeleton->nodes;
  skeleton_node_t *root = cvector_as(nodes, 0, skeleton_node_t);
  matrix4f identity;
  matrix4f_set_identity(&identity);
  adjust_child_bind_pose_transform(skinned_mesh, skeleton, root, identity);
}


static
void
insert_bone_weight(
  bone_id_weight_t *bone_weight,
  uint32_t bone,
  float weight)
{
  // sorted insert, or replacement
  // find if there are any available spots.
  const uint32_t size = sizeof(bone_weight->id)/sizeof(uint32_t);
  for (uint32_t i = 0; i < size; ++i) {
    if (bone_weight->weight[i] > weight)
      continue;

    // i is at an index where the stored weight is less than weight
    // we shift and then insert our value.
    for (uint32_t j = size - 1; j > i; --j) {
      bone_weight->id[j] = bone_weight->id[j - 1];
      bone_weight->weight[j] = bone_weight->weight[j - 1];
    }

    bone_weight->id[i] = bone;
    bone_weight->weight[i] = weight;
    break;
  }
}

static
void
calculate_vertex_to_bone(
  skinned_mesh_asset_t *skinned_mesh,
  const uint32_t vertex_count)
{
  cvector_setup2(&skinned_mesh->vertex_to_bones, bone_id_weight_t);
  cvector_resize(&skinned_mesh->vertex_to_bones, vertex_count);

  for (uint32_t i = 0; i < skinned_mesh->vertex_to_bones.size; ++i) {
    bone_id_weight_t *values = cvector_as(
      &skinned_mesh->vertex_to_bones, i, bone_id_weight_t);
    const uint32_t size = sizeof(values->id)/sizeof(uint32_t);
    for (uint32_t j = 0; j < size; ++j) {
      values->id[j] = (uint32_t)-1;     // optional, we only check the weight
      values->weight[j] = -1.f;
    }
  }

  for (uint32_t i = 0; i < skinned_mesh->bones.size; ++i) {
    skinned_mesh_bone_t *bone = cvector_as(
      &skinned_mesh->bones, i, skinned_mesh_bone_t);

    for (uint32_t j = 0; j < bone->vertex_weights.size; ++j) {
      bone_vertex_weight_t *data = cvector_as(
        &bone->vertex_weights, j, bone_vertex_weight_t);

      bone_id_weight_t *vertex_weight = cvector_as(
        &skinned_mesh->vertex_to_bones, data->vertex_id, bone_id_weight_t);

      insert_bone_weight(vertex_weight, i, data->weight);
    }
  }
}

static
void
import_skinned_meshes(
  const aiScene *pScene,
  const std::string &source_file,
  const std::string &target_dir,
  bool import_skeleton)
{
  for (uint32_t i = 0; i < pScene->mNumMeshes; ++i) {
    aiMesh *pMesh = pScene->mMeshes[i];

    if (!pMesh->HasBones())
      continue;

    mesh_import_info_t info = import_mesh(pScene, i, source_file, target_dir);
    skinned_mesh_asset_t mesh = {};

    // point to the mesh we already serialized.
    mesh.mesh.type_id = get_type_id(mesh_asset_t);
    std::string path = construct_asset_path(
      target_dir, mesh_asset_get_dir, pMesh->mName.C_Str());
    cstring_setup2(&mesh.mesh.path, path.c_str());

    copy_bone_data(&mesh, pMesh, pScene);
    skeleton_asset_t skeleton = {};
    read_skeleton(&skeleton, &mesh, pMesh, pScene);
    adjust_skinned_mesh_node_bind_pose_transform(&mesh, &skeleton);
    calculate_vertex_to_bone(&mesh, info.vertex_count);

    if (import_skeleton) {
      write_to_file(
        target_dir,
        &skeleton,
        skeleton_asset_serialize,
        skeleton_asset_get_dir,
        pMesh->mName.C_Str(), "bin");
    }

    write_to_file(
      target_dir,
      &mesh,
      skinned_mesh_asset_serialize,
      skinned_mesh_asset_get_dir,
      pMesh->mName.C_Str(), "bin");

    skeleton_asset_cleanup(&skeleton, &g_default_allocator);
    skinned_mesh_asset_cleanup(&mesh, &g_default_allocator);
  }
}

void
import_skinned_meshes(
  const std::string &source_file,
  const std::string &target_dir,
  bool import_skeleton)
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
    import_skinned_meshes(
      pScene, source_file, target_dir, import_skeleton);
}
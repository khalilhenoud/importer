/**
 * @file animations.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <cassert>
#include <cstdint>
#include <iostream>
#include <importer/animations.h>
#include <importer/utils.h>
#include <animation/animation_asset.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <library/allocator/allocator.h>
#include <library/streams/binary_stream.h>
#include <library/string/cstring.h>
#include <library/containers/cvector.h>
#include <library/type_registry/type_registry.h>


static
void
populate_animation(
  aiAnimation *aiAnim,
  animation_asset_t *anim)
{
  cstring_setup2(&anim->name, aiAnim->mName.C_Str());
  anim->duration = (float)aiAnim->mDuration;
  anim->ticks_per_second = aiAnim->mTicksPerSecond;

  cvector_setup2(&anim->channels, animation_node_t);
  cvector_resize(&anim->channels, aiAnim->mNumChannels);

  for (uint32_t i = 0; i < aiAnim->mNumChannels; ++i) {
    animation_node_t *anim_node = cvector_as(
      &anim->channels, i, animation_node_t);
    aiNodeAnim *aiNodeAnim = aiAnim->mChannels[i];

    cstring_setup2(&anim_node->name, aiNodeAnim->mNodeName.C_Str());
    cvector_setup2(&anim_node->position_keys, position_key_t);
    cvector_resize(&anim_node->position_keys, aiNodeAnim->mNumPositionKeys);

    for (uint32_t k = 0; k < aiNodeAnim->mNumPositionKeys; ++k) {
      position_key_t *position = cvector_as(
        &anim_node->position_keys, k, position_key_t);
      aiVectorKey *aiPosition = aiNodeAnim->mPositionKeys + k;
      position->time = (float)aiPosition->mTime;
      copy_vec3(&position->value, &aiPosition->mValue, AI_SUCCESS);
    }

    cvector_setup2(&anim_node->rotation_keys, rotation_key_t);
    cvector_resize(&anim_node->rotation_keys, aiNodeAnim->mNumRotationKeys);

    for (uint32_t k = 0; k < aiNodeAnim->mNumRotationKeys; ++k) {
      rotation_key_t *rotation = cvector_as(
        &anim_node->rotation_keys, k, rotation_key_t);
      aiQuatKey *aiRotation = aiNodeAnim->mRotationKeys + k;
      rotation->time = (float)aiRotation->mTime;
      copy_quat(&rotation->value, &aiRotation->mValue, AI_SUCCESS);
    }

    cvector_setup2(&anim_node->scale_keys, scale_key_t);
    cvector_resize(&anim_node->scale_keys, aiNodeAnim->mNumScalingKeys);

    for (uint32_t k = 0; k < aiNodeAnim->mNumScalingKeys; ++k) {
      scale_key_t *scale = cvector_as(&anim_node->scale_keys, k, scale_key_t);
      aiVectorKey *aiScale = aiNodeAnim->mScalingKeys + k;
      scale->time = (float)aiScale->mTime;
      copy_vec3(&scale->value, &aiScale->mValue, AI_SUCCESS);
    }
  }
}

static
void
import_animations(
  const aiScene *pScene,
  const std::string &source_file,
  const std::string &target_dir)
{
  for (uint32_t i = 0; i < pScene->mNumMeshes; ++i) {
    aiAnimation *aiAnim = pScene->mAnimations[i];

    animation_asset_t animation = {};
    populate_animation(aiAnim, &animation);

    write_to_file(
      target_dir,
      &animation,
      animation_asset_serialize,
      animation_asset_get_dir,
      aiAnim->mName.C_Str(), "bin");

    animation_asset_cleanup(&animation, &g_default_allocator);
  }
}

void
import_animations(
  const std::string &source_file,
  const std::string &target_dir)
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
    import_animations(pScene, source_file, target_dir);
}
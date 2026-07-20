/**
 * @file main.cpp
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2023-07-26
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <functional>
#include <iostream>
#include <malloc.h>
#include <vector>
#include <importer/importer.h>
#include <importer/utils.h>
#include <library/allocator/allocator.h>


static std::vector<uintptr_t> allocated;

void *
allocate(size_t size)
{
  void *block = malloc(size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void *
container_allocate(size_t count, size_t elem_size)
{
  void *block = calloc(count, elem_size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void *
reallocate(void *block, size_t size)
{
  void *tmp = realloc(block, size);
  assert(tmp);

  uintptr_t item = (uintptr_t)block;
  auto iter = std::find(allocated.begin(), allocated.end(), item);
  assert(iter != allocated.end());
  allocated.erase(iter);

  block = tmp;
  allocated.push_back(uintptr_t(block));

  return block;
}

void
free_block(void *block)
{
  allocated.erase(
    std::remove_if(
      allocated.begin(),
      allocated.end(),
      [=](uintptr_t elem) { return (uintptr_t)block == elem; }),
    allocated.end());
  free(block);
}

int
main(int argc, char *argv[])
{
  allocator_t allocator;
  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = nullptr;
  allocator.mem_realloc = reallocate;

  // NOTE: the tools_folder is going to be specified in the batch file, or
  // copied into the executable folder.
  assert(argc >= 3 && "incorrect number of arguments!");
  std::string source_file = argv[1];
  std::string target_dir = argv[2];

  uint32_t result = import(source_file, target_dir, &allocator);
  assert(result);

  assert(allocated.size() == 0);
  return 0;
}
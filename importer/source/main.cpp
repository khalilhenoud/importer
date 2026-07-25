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
#include <cassert>
#include <importer/importer.h>


int
main(int argc, char *argv[])
{
  // NOTE: the tools_folder is going to be specified in the batch file, or
  // copied into the executable folder.
  assert(argc >= 3 && "incorrect number of arguments!");
  std::string source_file = argv[1];
  std::string target_dir = argv[2];

  import(source_file, target_dir);
}
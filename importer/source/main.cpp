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


// TODO(@khalil): provide an argument that dictates the type of asset we are
// parsing. This is too cumbersome and error prone.
int
main(int argc, char *argv[])
{
  // NOTE: the tools_folder is going to be specified in the batch file, or
  // copied into the executable folder.
  assert(argc >= 3 && "incorrect number of arguments!");
  if (argc == 3) {
    std::string source_file = argv[1];
    std::string target_dir = argv[2];

    import(source_file, target_dir);
  } else if (argc == 4) {
    std::string source_file1 = argv[1];
    std::string source_file2 = argv[2];
    std::string target_dir = argv[3];

    import(source_file1, source_file2, target_dir);
  }
}
/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3, (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.gnu.org/licenses/agpl-3.0.en.html#license-text
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/

#include <filesystem>
#include <string>
#include <iostream>

int main() {
//   std::filesystem::path source_path = "D:/Workspace/webrtch265/src";
//   std::filesystem::path target_path = "D:/Dev/OpenZICloudRenderServer/thirdparty/RTCH265/include";
//   for (const auto& entry : std::filesystem::recursive_directory_iterator(source_path)) {
//     if (entry.is_regular_file()) {
//       if (entry.path().extension() == ".h") {
//         const auto relative_path = std::filesystem::relative(entry.path(), source_path);
//         // std::cout << relative_path.u8string() << std::endl;
//         const auto file_parent_path = target_path / relative_path.parent_path();
//         const auto file_path = target_path / relative_path;
//         // std::cout << file_parent_path.u8string() << std::endl;
//         if (file_parent_path.filename() == "test"
//           || file_parent_path.filename() == "tests") {
//             // std::cout << file_parent_path << std::endl;
//         } else {
//           if (!std::filesystem::exists(file_parent_path)) {
//             std::filesystem::create_directories(file_parent_path);
//           }
//           // copy
//           std::filesystem::copy_file(entry.path(), file_path, std::filesystem::copy_options::overwrite_existing);
//           std::cout << "copyto:" << file_path << std::endl;
//         }
//       }
//     }
//   }
  
  std::filesystem::path source_path = "D:/Dev/OpenZICloudRenderServer/thirdparty/RTCH265/include";
  for (const auto& entry : std::filesystem::recursive_directory_iterator(source_path)) {
    if (entry.is_directory()) {
      if (entry.path().filename() == "test" || entry.path().filename() == "tests") {
        std::cout << "remove:" << entry.path() << std::endl;
        std::filesystem::remove_all(entry.path());
      }
    }
  }
}
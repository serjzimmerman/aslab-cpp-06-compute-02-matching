/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <tsimmerman.ss@phystech.edu>, <alex.rom23@mail.ru> wrote this file.  As long as you
 * retain this notice you can do whatever you want with this stuff. If we meet
 * some day, and you think this stuff is worth it, you can buy me a beer in
 * return.
 * ----------------------------------------------------------------------------
 */

#include <CL/cl.h>
#include <exception>
#include <iostream>
#include <unordered_map>

#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>

int main(int argc, char *argv[]) try {
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  std::cout << "Number of platforms = " << platforms.size() << "\n";
  for (auto &p : platforms) {
    std::cout << " --------\n";
    std::cout << "Name: " << p.getInfo<CL_PLATFORM_NAME>() << "\n";

    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    std::cout << "Number of devices = " << devices.size() << "\n";
    for (unsigned i = 0; auto &d : devices) {
      std::cout << "Device [" << i++ << "] properties:\n";
      std::cout << "  Device name: " << d.getInfo<CL_DEVICE_NAME>() << "\n";

      static const std::unordered_map<int, std::string> type_map = {
          {CL_DEVICE_TYPE_GPU, "CL_DEVICE_TYPE_GPU"},
          {CL_DEVICE_TYPE_CPU, "CL_DEVICE_TYPE_CPU"},
          {CL_DEVICE_TYPE_ACCELERATOR, "CL_DEVICE_TYPE_ACCELERATOR"}};

      auto type = d.getInfo<CL_DEVICE_TYPE>();
      if (type_map.contains(type)) {
        std::cout << "  Device type: " << type_map.at(type) << "\n";
      } else {
        std::cout << "  Device type: Unknown\n";
      }
    }
  }

} catch (cl::Error &e) {
  std::cerr << "OpenCL error: " << e.what() << "\n";
} catch (std::exception &e) {
  std::cerr << "Encountered error: " << e.what() << "\n";
} catch (...) {
  std::cerr << "Unknown error\n";
}
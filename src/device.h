// Copyright 2017 Neverware
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/*
A linkedlist of device info passed between the C and C++ layer
*/
#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>
#include <string>
#include <vector>

// TODO(kendall): rename to something more thoughtful
class DeviceGuy {
 public:
  DeviceGuy(const DeviceGuy& other) = default;
  DeviceGuy(uint32_t device_num, const std::string name);

  std::string toString() const;

  bool operator==(const DeviceGuy& other) const;
  bool operator<(const DeviceGuy& other) const;

  uint32_t device_num = 0;
  std::string name;
};

typedef std::vector<DeviceGuy> DeviceGuyList;

DeviceGuy findDevice(const DeviceGuyList& devices, const uint32_t device_num);

#endif /* DEVICE_H */

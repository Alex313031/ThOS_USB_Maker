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

#include "device.h"

#include <string>

DeviceGuy::DeviceGuy(uint32_t device_num_in, const std::string name_in)
    : device_num(device_num_in), name(name_in) {}

bool DeviceGuy::operator==(const DeviceGuy& other) const {
  return (name == other.name) && (device_num == other.device_num);
}

bool DeviceGuy::operator<(const DeviceGuy& other) const {
  if (name == other.name) {
    return device_num < other.device_num;
  } else {
    return name < other.name;
  }
}

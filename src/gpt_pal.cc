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

#include "gpt_pal.h"

// We use gdisk to clean up the GPT such that Windows is happy writing to
// the disk
#include "../gdisk/gpt.h"
#include "../gdisk/parttypes.h"

// FIXME: temp
#include <stdio.h>

class PalData : public GPTData {
 public:
  PalData();
  WhichToUse UseWhichPartitions(void) override;
  void ClearDisk();
};

PalData::PalData() : GPTData() {}

WhichToUse PalData::UseWhichPartitions(void) {
  // The disk may be in a weird state, but we are about to reformat it.
  // Just always use a new partition table.
  return use_new;
}

void PalData::ClearDisk() {
  //BlankPartitions();
  //SaveGPTData(true);
  DisplayGPTData();
  int newPartNum = 0;
  uint64_t low = FindFirstInLargest();
  Align(&low);
  uint64_t high = FindLastInFree(low);
  uint64_t startSector = low;
  uint64_t endSector = high;
  // first sector is correct; could just keep the above.
  //uint64_t startSector = 2048;
  //uint64_t startSector = 4096;
  // this is what i get when i use gparted on this disk.
  //uint64_t endSector = 30000000;
  // start: 2048
  // end: 0
  // gparted says first sector: 0; last: 30736384
  // ^ but maybe that just means no partition/free space starts at 2048?
  printf("start:%d\nend:%d\n", startSector, endSector);
  int ret = CreatePartition(newPartNum, startSector, endSector);
  // ^ returns a 1 (success)
  printf("create partition returned %d\n", ret);
  //partitions[0].SetType(0x0b00);  // make it fat32
  printf("part type before is %s\n", partitions[0].GetTypeName().c_str());
  partitions[0].SetFirstLBA(startSector);
  partitions[0].SetLastLBA(endSector);
  partitions[0].SetType(0x0b00);  // make it fat32
  partitions[0].RandomizeUniqueGUID();
  printf("part type after is %s\n", partitions[0].GetTypeName().c_str());
  //printf("part type is %s\n", partitions[0].GetType().ShowAllTypes(0);
  // arg is 'quiet'
  DisplayGPTData();
  SaveGPTData(true);
}

bool clearMbrGpt(const char* physical_path) {
  std::string physical_path_str(physical_path);
  PalData gptdata;
  // set the physical path for this GPT object to act on
  gptdata.LoadPartitions(std::string(physical_path));
  int quiet = true;
  // attempt to fix any gpt/mbr problems by setting to a sane, empty state
  gptdata.SaveGPTData(quiet);
  int problems = gptdata.Verify();
  printf("cleared mbr/gpt\n");
  if (problems > 0) {
    return false;
  } else {
    return true;
  }
}

// FIXME: this should make a fat32 partition
bool makeEmptyPartition(const char* physical_path) {
  char* newPartInfo;
  PalData gptdata;
  gptdata.LoadPartitions(std::string(physical_path));
  gptdata.ClearDisk();

  int problems = gptdata.Verify();
  free(newPartInfo);
  // TODO: unclear if we care about problems in this regard
  if (problems > 0) {
    printf("there were problems\n");
    return true;
  } else {
    printf("there were no problems\n");
    return false;
  }
}

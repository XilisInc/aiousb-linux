#include <iostream>
#include <iomanip>
#include <cstring>
#include <list>
#include <algorithm>
#include <chrono>
#include <thread>

#include "aiousb.h"

static const std::list<uint16_t> PIDS_WITH_PER_BIT = {0x8004};

#include "AiousbSamples.inc"

int main (int argc, char **argv)
{
  int Status;
  aiousb_device_handle Device;
  uint32_t DioBytes;
  uint32_t Pid;
  bool PerBitControl = false;

  std::cout<<"ACCES AIOUSB-Linux DIO sample"<<std::endl;

  aiousb_init();

  //Try to get a device handle.
  Status = SampleGetDeviceHandle(argc, argv, &Device);

  if ( 0 != Status )
  {
    std::cout << "Unable to get device handle" << std::endl;
    SampleUsage(argc, argv);
    exit (-1);
  }

  uint32_t nameSize = 255;
  char name[nameSize];

  // DIO bytes is something we can query from the aiousb library so we see how
  // many are available on this device.
  aiousb_query_device_info(Device, &Pid, &nameSize, name, &DioBytes, nullptr);

  if ( 0 == DioBytes )
  {
    std::cout << name << " doesn't support DIO" << std::endl;
    exit (-1);
  }

  std::cout << name << " detected, with " << DioBytes << " digital bytes" << std::endl;

  //Check if this device has direction control per bit.
  if (std::find(PIDS_WITH_PER_BIT.begin(), PIDS_WITH_PER_BIT.end(), Pid) != PIDS_WITH_PER_BIT.end())
  {
    PerBitControl = true;
  }

  //  Config controls if the bytes are input or output. 1 bit per byte or group
  //  on devices that don't have per bit control
  uint8_t *Config;
  size_t ConfigBytes = PerBitControl ? DioBytes : DioBytes % 8 + 1;
  // Data is value of the bits read or written
  uint8_t *Data;

  Config = new uint8_t[ConfigBytes];
  Data = new uint8_t[DioBytes];

  // First we'll set all the bits for output and toggle them a couple of times.
  memset(Config, 0xff, ConfigBytes);
  memset(Data, 0xff, DioBytes);

  std::cout << "Configuring DIO for output and setting high" << std::endl;
  aiousb_dio_configure(Device, false, Config, Data);

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  std::cout << "Setting output to low" << std::endl;
  memset(Data, 0, DioBytes);
  aiousb_dio_write_all(Device, Data);

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  std::cout << "Setting output to high" << std::endl;
  memset(Data, 0xff, DioBytes);
  aiousb_dio_write_all(Device, Data);

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  std::cout << "Configuring DIO for input" << std::endl;
  memset(Config, 0, ConfigBytes);
  aiousb_dio_configure(Device, false, Config, Data);
  aiousb_dio_read_all(Device, Data);

  std::cout << "Read: ";
  for (unsigned int i = 0 ; i < DioBytes ; i++)
  {
    std::cout << "bits "<< std::dec << i*8 << "-" << (i+1)*8-1 << ": 0x" << std::hex << std::setw(2) << std::setfill('0') << int(Data[i]) << "   ";
  }
  std::cout << std::endl;

  std::cout << "Sleeping for 5 seconds; change the input state before 2nd read." << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  std::cout << "Performing second read" << std::endl;
  aiousb_dio_read_all(Device, Data);
  std::cout << "Read: ";
  for (unsigned int i = 0 ; i < DioBytes ; i++)
  {
    std::cout << "bits "<< std::dec << i*8 << "-" << (i+1)*8-1 << ": 0x" << std::hex << std::setw(2) << std::setfill('0') << int(Data[i]) << "   ";
  }
  std::cout << std::endl;

  delete []Config;
  delete []Data;

  return 0;
}

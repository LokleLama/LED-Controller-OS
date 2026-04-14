#include "Config.h"
#include "Console.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <iostream>
#include <memory>
#include <sys/types.h>
#include <vector>

#include "Commands/BootCommand.h"

#include "Commands/EchoCommand.h"
#include "Commands/EnvCommand.h"
#include "Commands/GetCommand.h"
#include "Commands/HelpCommand.h"
#include "Commands/LedCommand.h"
#include "Commands/SetCommand.h"
#include "Commands/TimeCommand.h"
#include "Commands/VersionCommand.h"
#include "Commands/MemInfoCommand.h"
#include "Commands/CreateVarCommand.h"

#include "Commands/DirCommand.h"
#include "Commands/ChangeDirCommand.h"
#include "Commands/MakeDirCommand.h"
#include "Commands/CatCommand.h"
#include "Commands/ExecCommand.h"
#include "Commands/FSInfoCommand.h"
#include "Commands/StoreCommand.h"

#include "Commands/ReadCommand.h"
#include "Commands/WriteCommand.h"

#include "Commands/MakeFilesystemCommand.h"

#include "Commands/DeviceCommand.h"

#include "Commands/TaskCommand.h"
#include "Commands/KillCommand.h"

#include "BackgroundTasks/StartCommand.h"

#include "VariableStore/VariableStore.h"
#include "deviceController/DeviceRepository.h"

#include "devices/USBUARTDevice.h"

#include "RTC/PicoTime.h"

#include "Mainloop.h"

static std::shared_ptr<PicoTime> picoTime;

int main() {
  stdio_init_all();

  tusb_init();

  auto& mainloop = Mainloop::getInstance();
  auto& variableStore = VariableStore::getInstance();

  auto fs = std::make_shared<SPFS>();
  auto rootDir = fs->searchFileSystem(SPFS_FLASH_OFFSET, SPFS_FLASH_OFFSET + SPFS_FLASH_SIZE);
  if (!rootDir) {
    if(!fs->createNewFileSystem(SPFS_FLASH_OFFSET, SPFS_FLASH_SIZE, "LEDControllerFS", "root")){
      fs = nullptr;
    }
  }
  variableStore.addVariable("var.fs_offset", SPFS_FLASH_OFFSET);
  variableStore.addVariable("var.fs_size", SPFS_FLASH_SIZE);

  Console console(variableStore, fs);
  DeviceRepository deviceRepo(console);

  deviceRepo.addDevice(std::make_shared<USBUARTDevice>(UART_INTERFACE_NUMBER));

  picoTime = std::make_shared<PicoTime>();

  console.registerCommand(std::make_shared<BootCommand>());
  
  console.registerCommand(std::make_shared<VersionCommand>());
  console.registerCommand(std::make_shared<EchoCommand>());

  console.registerCommand(std::make_shared<SetCommand>(variableStore));
  console.registerCommand(std::make_shared<GetCommand>(variableStore));
  console.registerCommand(std::make_shared<EnvCommand>(variableStore, console));
  console.registerCommand(std::make_shared<CreateVarCommand>(variableStore));

  console.registerCommand(std::make_shared<HelpCommand>(console));

  console.registerCommand(std::make_shared<DirCommand>(console));
  console.registerCommand(std::make_shared<ChangeDirCommand>(console));
  console.registerCommand(std::make_shared<MakeDirCommand>(console));
  console.registerCommand(std::make_shared<CatCommand>(console));
  console.registerCommand(std::make_shared<ExecCommand>(console));
  console.registerCommand(std::make_shared<FSInfoCommand>(console));
  console.registerCommand(std::make_shared<StoreCommand>(console));

  console.registerCommand(std::make_shared<ReadCommand>());
  console.registerCommand(std::make_shared<WriteCommand>());

  console.registerCommand(std::make_shared<MakeFilesystemCommand>(console));
  console.registerCommand(std::make_shared<MemInfoCommand>());

  console.registerCommand(std::make_shared<DeviceCommand>(variableStore, deviceRepo));

  console.registerCommand(std::make_shared<TaskCommand>());
  console.registerCommand(std::make_shared<KillCommand>(mainloop));

  console.registerCommand(std::make_shared<TimeCommand>(picoTime));
  console.registerCommand(std::make_shared<StartCommand>(picoTime));
  console.registerCommand(std::make_shared<LedCommand>(console, deviceRepo));

  variableStore.addVariable("init-script", "startup.sh");
  variableStore.addVariable("?", 0)->setSystemVariable();

  console.EnqueueCommand("env load");
  console.EnqueueCommand("exec ${init-script}");

  mainloop.registerRegularTask(&console);

  mainloop.start();

  return 0;
}

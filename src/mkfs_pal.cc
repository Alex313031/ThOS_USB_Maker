
#include "mkfs_pal.h"

#include <windows.h>
#include <inttypes.h>
#include <winioctl.h>       // for MEDIA_TYPE

#include "log.h"
#include "msapi_utf8.h"

HMODULE  OpenedLibraryHandle;
static __inline HMODULE GetLibraryHandle() {
  return OpenedLibraryHandle;
}

/* Callback command types (some errorcode were filled from HPUSBFW V2.2.3 and their
   designation from msdn.microsoft.com/en-us/library/windows/desktop/aa819439.aspx */
typedef enum {
  FCC_PROGRESS,
  FCC_DONE_WITH_STRUCTURE,
  FCC_UNKNOWN2,
  FCC_INCOMPATIBLE_FILE_SYSTEM,
  FCC_UNKNOWN4,
  FCC_UNKNOWN5,
  FCC_ACCESS_DENIED,
  FCC_MEDIA_WRITE_PROTECTED,
  FCC_VOLUME_IN_USE,
  FCC_CANT_QUICK_FORMAT,
  FCC_UNKNOWNA,
  FCC_DONE,
  FCC_BAD_LABEL,
  FCC_UNKNOWND,
  FCC_OUTPUT,
  FCC_STRUCTURE_PROGRESS,
  FCC_CLUSTER_SIZE_TOO_SMALL,
  FCC_CLUSTER_SIZE_TOO_BIG,
  FCC_VOLUME_TOO_SMALL,
  FCC_VOLUME_TOO_BIG,
  FCC_NO_MEDIA_IN_DRIVE,
  FCC_UNKNOWN15,
  FCC_UNKNOWN16,
  FCC_UNKNOWN17,
  FCC_DEVICE_NOT_READY,
  FCC_CHECKDISK_PROGRESS,
  FCC_UNKNOWN1A,
  FCC_UNKNOWN1B,
  FCC_UNKNOWN1C,
  FCC_UNKNOWN1D,
  FCC_UNKNOWN1E,
  FCC_UNKNOWN1F,
  FCC_READ_ONLY_MODE,
} FILE_SYSTEM_CALLBACK_COMMAND;

typedef BOOLEAN (__stdcall *FILE_SYSTEM_CALLBACK)(
  FILE_SYSTEM_CALLBACK_COMMAND Command,
  ULONG                        Action,
  PVOID                        pData
);

/* Parameter names aligned to
   http://msdn.microsoft.com/en-us/library/windows/desktop/aa819439.aspx */
typedef VOID (WINAPI *FormatEx_t)(
  WCHAR*               DriveRoot,
  MEDIA_TYPE           MediaType,   // See WinIoCtl.h
  WCHAR*               FileSystemTypeName,
  WCHAR*               Label,
  BOOL                 QuickFormat,
  ULONG                DesiredUnitAllocationSize,
  FILE_SYSTEM_CALLBACK Callback
);
FormatEx_t pfFormatEx = NULL;

/*
 * FormatEx callback. Return FALSE to halt operations
 */
static BOOLEAN __stdcall FormatExCallback(FILE_SYSTEM_CALLBACK_COMMAND Command, DWORD, PVOID)
{
  switch(Command) {
  case FCC_PROGRESS:
    LOG_INFO << "still formatting drive...";
    break;
  case FCC_CLUSTER_SIZE_TOO_SMALL:
    LOG_WARNING << "cluster size too small";
    break;
  case FCC_CLUSTER_SIZE_TOO_BIG:
    LOG_WARNING << "cluster size too big";
    break;
  case FCC_DONE:
    LOG_INFO << "finished formatting drive";
    // FIXME: when do i free the library safely?
    //FreeLibrary(OpenedLibraryHandle);
    break;
  case FCC_READ_ONLY_MODE:
    LOG_WARNING << "read-only mode";
    break;
  case FCC_DEVICE_NOT_READY:
    LOG_WARNING << "device not ready";
    break;
  default:
    LOG_WARNING << "unknown callback case";
    break;
  }
  return true;
}


void makeFilesystem(char* logical_path) {
  LOG_WARNING << "making filesystem...";
  // LoadLibrary("fmifs.dll") appears to changes the locale, which can lead to
  // problems with tolower(). Make sure we restore the locale. For more details,
  // see http://comments.gmane.org/gmane.comp.gnu.mingw.user/39300
  char* locale = setlocale(LC_ALL, NULL);
  pfFormatEx = (FormatEx_t) GetProcAddress(LoadLibraryA("fmifs.dll"), "FormatEx");
  setlocale(LC_ALL, locale);

  // TODO: make sure the name i'm passing is this
  wchar_t* logical_path_windows = utf8_to_wchar(logical_path);
  pfFormatEx(logical_path_windows, RemovableMedia, L"FAT32", L"", /*quick*/true, /*clustersize*/4096, FormatExCallback);
  LOG_INFO << "sent request to make filesystem...";
}

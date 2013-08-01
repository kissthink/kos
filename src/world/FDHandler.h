/******************************************************************************
    Copyright 2013 Behrooz Shafia

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef FDHandler_H_
#define FDHandler_H_

#include <cstddef>
#include <cstdint>

#define MAX_FILE_NUM 1000

class FDHandler {
  enum File_Status { OPEN,CLOSED };

  struct File_Descriptor_Entry {
    class File;              // Forward Declare Class File
    File* fInfo;             // Pointer to Kernel File System
    File_Status fileStatus;  // File Status
    File_Descriptor_Entry() : fInfo(NULL), fileStatus(File_Status::CLOSED) {}
  };

  static File_Descriptor_Entry fileDesArr[MAX_FILE_NUM]; // FD array

public:
  static int openHandle(const char *path, int oflag, int mode);
  static size_t readHandle(int fd, char* buf, int len);
  static off_t lseekHandle(int fd, off_t offset, int whence);
  static int closeHandle(int fd);
};

#endif /* FDHandler_H_ */

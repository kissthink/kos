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
#include "kern/Kernel.h"
#include "world/FDHandler.h"
#include "world/File.h"

FDHandler::File_Descriptor_Entry FDHandler::fileDesArr[MAX_FILE_NUM];

int FDHandler::openHandle(const char *path, int oflag, int mode) {
  map<kstring, File*, less<kstring>, KernelAllocator<kstring>>::iterator it;
  it = kernelFS.find(path);
  if (it == kernelFS.end()) return -1;         // no such file
  for (int i = 0; i < MAX_FILE_NUM; ++i) {     // file exists
    if (fileDesArr[i].fileStatus == File_Status::CLOSED) {
      FDHandler::fileDesArr[i].fileStatus = File_Status::OPEN;
      FDHandler::fileDesArr[i].File_Descriptor_Entry::fInfo =
        (FDHandler::File_Descriptor_Entry::File*) it->second;
      return i;
    }
  }
  StdErr.outln("No more available FD");
  return -1;
}

size_t FDHandler::readHandle(int fd, char* buf, int len) {
  if (fd >= MAX_FILE_NUM && fd < 0) return -1; // no such file
  File *f = (File*) FDHandler::fileDesArr[fd].fInfo;
  return f->read(buf, len);
}

off_t FDHandler::lseekHandle(int fd, off_t offset, int whence) {
  if (fd >= MAX_FILE_NUM && fd < 0) return -1; // no such file
  File *f = (File*) FDHandler::fileDesArr[fd].fInfo;
  return f->lseek(offset, whence);
}

int FDHandler::closeHandle(int fd) {
  if (fd >= MAX_FILE_NUM && fd < 0) return -1; // no such file
  FDHandler::fileDesArr[fd].fileStatus = File_Status::CLOSED;
  FDHandler::fileDesArr[fd].fInfo = NULL;
  return 0;
}

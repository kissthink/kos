/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _Errors_h_
#define _Errors_h_ 1

// This namespace is mapped to posix errno's where possible. Keep it that way!
namespace Error
{
  enum PosixError
  {
    NoError              =0,
    NotEnoughPermissions =1,
    DoesNotExist         =2,
    NoSuchProcess        =3,
    Interrupted          =4,
    IoError              =5,
    NoSuchDevice         =6,
    TooBig               =7,
    ExecFormatError      =8,
    BadFileDescriptor    =9,
    NoChildren           =10,
    NoMoreProcesses      =11,
    OutOfMemory          =12,
    PermissionDenied     =13,
    BadAddress           =14,
    DeviceBusy           =16,
    FileExists           =17,
    CrossDeviceLink      =18,
    NotADirectory        =20,
    IsADirectory         =21,
    InvalidArgument      =22,
    TooManyOpenFiles     =23,
    NotAConsole          =25,
    FileTooLarge         =27,
    NoSpaceLeftOnDevice  =28,
    IllegalSeek          =29,
    ReadOnlyFilesystem   =30,
    BrokenPipe           =32,
    LoopExists           =92,
    OperationNotSupported=95,
    TimedOut             =116,
    InProgress           =119,
    Already              =120,
    IsConnected          =127,
    NotSupported         =134,
    Unimplemented        =88
  };
}

#endif /* _Errors_h_ */

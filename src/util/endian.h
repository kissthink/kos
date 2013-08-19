#ifndef _Endian_h_
#define _Endian_h_ 1

#define BYTE_SWAP8(x)  (x)
#define BYTE_SWAP16(x) (((x&0xff00)>>8)|((x&0x00ff)<<8))
#define BYTE_SWAP32(x) (((x&0xff000000)>>24)|((x&0x00ff0000)>>8)|((x&0x0000ff00)<<8)|((x&0x000000ff)<<24))
#define BYTE_SWAP64(x) (((x&0xff00000000000000)>>56)|((x&0x00ff000000000000)>>40)|((x&0x0000ff0000000000)>>24)|((x&0x000000ff00000000)>>8)|((x&0x00000000ff000000)<<8)|((x&0x0000000000ff0000)<<24)|((x&0x000000000000ff00)<<40)|((x&0x00000000000000ff)<<56))

#define LITTLE_TO_HOST8(x)    (x)
#define LITTLE_TO_HOST16(x)   (x)
#define LITTLE_TO_HOST32(x)   (x)
#define LITTLE_TO_HOST64(x)   (x)

#define HOST_TO_LITTLE8(x)    (x)
#define HOST_TO_LITTLE16(x)   (x)
#define HOST_TO_LITTLE32(x)   (x)
#define HOST_TO_LITTLE64(x)   (x)

#define BIG_TO_HOST8(x)     BYTE_SWAP8((x))
#define BIG_TO_HOST16(x)    BYTE_SWAP16((x))
#define BIG_TO_HOST32(x)    BYTE_SWAP32((x))
#define BIG_TO_HOST64(x)    BYTE_SWAP64((x))

#define HOST_TO_BIG8(x)     BYTE_SWAP8((x))
#define HOST_TO_BIG16(x)    BYTE_SWAP16((x))
#define HOST_TO_BIG32(x)    BYTE_SWAP32((x))
#define HOST_TO_BIG64(x)    BYTE_SWAP64((x))

#endif /* _Endian_h_ */

#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;

s32 from_syscall_ret(s32 ret){
    if (ret >= 0) {
        return ret;
    }
    if (ret <= -4096) {
        return EINVAL;
    }
    return errno_to_wasi(ret); // TODO: should be negative?
}

// errno
// s32 errno_to_native(s32 errno){}
s32 errno_to_wasi(s32 clockid){
    switch(clockid) {
        case 0  : return 0;
        case EACCES  : return 2;
        case EADDRINUSE  : return 3;
        case EADDRNOTAVAIL  : return 4;
        case EBADF  : return 8;
        case ECONNABORTED  : return 13;
        case ECONNREFUSED  : return 14;
        case ECONNRESET  : return 15;
        case EEXIST  : return 20;
        case EFAULT  : return 21;
        case EFBIG  : return 22;
        case EHOSTUNREACH  : return 23;
        case EINVAL  : return 28;
        case EIO  : return 29;
        case EISCONN  : return 30;
        case EISDIR  : return 31;
        case ELOOP  : return 32;
        case ENETUNREACH  : return 40;
        case EMFILE  : return 41;
        case ENOSPC  : return 51;
        case ENOTDIR  : return 54;
        case ENOTEMPTY  : return 55;
        case ENOTSOCK  : return 57;
        case EOVERFLOW  : return 61;
        default : assert(false); // Unknown error? 
    }
}


// __WASI_ERRNO_NETUNREACH (UINT16_C(40))
// __WASI_ERRNO_NFILE (UINT16_C(41))
// __WASI_ERRNO_NOBUFS (UINT16_C(42))
// __WASI_ERRNO_NODEV (UINT16_C(43))
// __WASI_ERRNO_NOENT (UINT16_C(44))
// __WASI_ERRNO_NOEXEC (UINT16_C(45))
// __WASI_ERRNO_NOLCK (UINT16_C(46))
// __WASI_ERRNO_NOLINK (UINT16_C(47))
// __WASI_ERRNO_NOMEM (UINT16_C(48))
// __WASI_ERRNO_NOMSG (UINT16_C(49))
// __WASI_ERRNO_NOPROTOOPT (UINT16_C(50))
// __WASI_ERRNO_NOSPC (UINT16_C(51))
// __WASI_ERRNO_NOSYS (UINT16_C(52))
// __WASI_ERRNO_NOTCONN (UINT16_C(53))
// __WASI_ERRNO_NOTDIR (UINT16_C(54))
// __WASI_ERRNO_NOTEMPTY (UINT16_C(55))
// __WASI_ERRNO_NOTRECOVERABLE (UINT16_C(56))
// __WASI_ERRNO_NOTSOCK (UINT16_C(57))
// __WASI_ERRNO_NOTSUP (UINT16_C(58))
// __WASI_ERRNO_NOTTY (UINT16_C(59))
// __WASI_ERRNO_NXIO (UINT16_C(60))
// __WASI_ERRNO_OVERFLOW (UINT16_C(61))
// __WASI_ERRNO_OWNERDEAD (UINT16_C(62))
// __WASI_ERRNO_PERM (UINT16_C(63))
// __WASI_ERRNO_PIPE (UINT16_C(64))
// __WASI_ERRNO_PROTO (UINT16_C(65))
// __WASI_ERRNO_PROTONOSUPPORT (UINT16_C(66))
// __WASI_ERRNO_PROTOTYPE (UINT16_C(67))
// __WASI_ERRNO_RANGE (UINT16_C(68))
// __WASI_ERRNO_ROFS (UINT16_C(69))
// __WASI_ERRNO_SPIPE (UINT16_C(70))
// __WASI_ERRNO_SRCH (UINT16_C(71))


// oflags

s32 oflags_to_native(s32 flags){
    int out_flags = 0;
    if (flags & 1 != 0) 
        out_flags |= O_CREAT;
    if (flags & 1 << 1 != 0) 
        out_flags |= O_DIRECTORY;
    if (flags & 1 << 2 != 0) 
        out_flags |= O_EXCL;
    if (flags & 1 << 3 != 0) 
        out_flags |= O_TRUNC;
    if (flags & 1 << 4 != 0) 
        out_flags |= O_WRONLY;
    if (flags & 1 << 5 != 0) 
        out_flags |= O_RDWR;
    return out_flags;
}

s32 oflags_to_wasi(s32 flags){
    int out_flags = 0;
    if (flags | O_CREAT != 0)
        out_flags |= 1;
    if (flags | O_DIRECTORY != 0)
        out_flags |= 1 << 1;
    if (flags | O_EXCL != 0)
        out_flags |= 1 << 2;
    if (flags | O_TRUNC != 0)
        out_flags |= 1 << 3;
    if (flags | O_WRONLY != 0)
        out_flags |= 1 << 4;
    if (flags | O_RDWR != 0)
        out_flags |= 1 << 5;
    return out_flags;
}

// clockid

s32 clockid_to_native(s32 clockid){
    switch(clockid) {
        case 0  : return CLOCK_REALTIME;
        case 1  : return CLOCK_MONOTONIC;
        case 2  : return CLOCK_PROCESS_CPUTIME_ID;
        case 3  : return CLOCK_THREAD_CPUTIME_ID;
        default : return -1; 
    }
}

// advice

s32 advice_to_native(s32 advice){
    switch(advice) {
        case 0  : return POSIX_FADV_NORMAL;
        case 1  : return POSIX_FADV_SEQUENTIAL;
        case 2  : return POSIX_FADV_RANDOM;
        case 3  : return POSIX_FADV_WILLNEED;
        case 4  : return POSIX_FADV_DONTNEED; 
        case 5  : return POSIX_FADV_NOREUSE;
        default : return 0; // Unknown
    }
}

// s32 advice_to_wasi(s32 advice){
//     switch(advice) {
//         case S_IFBLK  : return 1;
//         case S_IFCHR  : return 2;
//         case S_IFDIR  : return 3;
//         case S_IFREG  : return 4;
//         case S_IFSOCK  : return 6; // 5 is Dgram, 6 is Stream
//         case S_IFLINK  : return 5;
//         default : return 0; // Unknown
//     }
// }

// filetype

s32 filetype_to_native(s32 filetype){
    switch(filetype & S_IFMT) {
        case 1  : return S_IFBLK;
        case 2  : return S_IFCHR;
        case 3  : return S_IFDIR;
        case 4  : return S_IFREG;
        case 5  : return S_IFSOCK; // 5 is Dgram, 6 is Stream
        case 6  : return S_IFSOCK;
        case 7  : return S_IFLINK; 
        default : return 0; // Unknown
    }
}

s32 filetype_to_wasi(s32 filetype){
    switch(filetype) {
        case S_IFBLK  : return 1;
        case S_IFCHR  : return 2;
        case S_IFDIR  : return 3;
        case S_IFREG  : return 4;
        case S_IFSOCK  : return 6; // 5 is Dgram, 6 is Stream
        case S_IFLINK  : return 5;
        default : return 0; // Unknown
    }
}

// fdflags
s32 fdflags_to_native(s32 flags){
    int out_flags = 0;
    if (flags & 1 != 0) 
        out_flags |= O_APPEND;
    if (flags & 1 << 1 != 0) 
        out_flags |= O_DSYNC;
    if (flags & 1 << 2 != 0) 
        out_flags |= O_NONBLOCK;
    if (flags & 1 << 3 != 0) 
        out_flags |= O_RSYNC;
    if (flags & 1 << 4 != 0) 
        out_flags |= O_SYNC;
    return out_flags;
}

s32 fdflags_to_wasi(s32 flags){
    int out_flags = 0;
    if (flags | O_APPEND != 0)
        out_flags |= 1;
    if (flags | O_DSYNC != 0)
        out_flags |= 1 << 1;
    if (flags | O_NONBLOCK != 0)
        out_flags |= 1 << 2;
    if (flags | O_RSYNC != 0)
        out_flags |= 1 << 3;
    if (flags | O_SYNC != 0)
        out_flags |= 1 << 4;
    return out_flags;
}



// lookupflags
s32 lookupflags_to_native(s32 flags){
    int out_flags = 0;
    if (flags & 1 != 0) 
        out_flags |= O_NOFOLLOW;
    return out_flags;
}

// s32 lookupflags_to_wasi(s32 flags){
// }


// fstflags
bool atim_flag(u16 flags){
    return flags & 1 != 0;
}

bool atim_now_flag(u16 flags){
    return flags & 2 != 0;
}

bool mtim_flag(u16 flags){
    return flags & 4 != 0;
}

bool mtim_now_flag(u16 flags){
    return flags & 8 != 0;
}


// sdflags
u8 sdflags_to_native(u8 flags){
    bool rd = flags & 1 != 0; // 0th bit set
    bool wr = flags & 2 != 0; // 1st bit set
    if (rd && wr) return SHUT_RDWR;
    if (rd) return SHUT_RD;
    if (wr) return SHUT_WR;
    return 0; 
}
//u8 sdflags_to_wasi(u8 flags){}


s32 sock_domain_to_native(s32 domain){
    if (domain == 1) {
        return AF_INET;
    }
    return -1;
}

s32 sock_type_to_native(s32 ty) {
    if (ty == 6) {
        return SOCK_STREAM;
    }
    if (ty == 5) {
        return SOCK_DGRAM;
    }
    return -1;
}

// //#[with_ghost_var(trace: &mut Trace)]
// pub fn sock_type_to_posix(ty: u32) -> RuntimeResult<i32> {
//     if ty == 6 {
//         return Ok(libc::SOCK_STREAM);
//     }
//     if ty == 5 {
//         return Ok(libc::SOCK_DGRAM);
//     }
//     return Err(RuntimeError::Enotsup);
// }



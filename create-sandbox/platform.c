#include "platform.h"
#include "sandbox.h"
#include "internal.h"
#include "config.h"

#include <ctype.h>              /* toupper() */
#include <fcntl.h>              /* open(), close(), O_RDONLY */
#include <math.h>               /* modfl(), lrintl() */
#include <pthread.h>            /* pthread_{create,join,...}() */
#include <signal.h>             /* kill(), SIG* */
#include <stdio.h>              /* read(), sscanf(), sprintf() */
#include <stdlib.h>             /* malloc(), free() */
#include <string.h>             /* memset(), strsep() */
#include <sys/queue.h>          /* SLIST_*() */
#include <sys/times.h>          /* struct tms, struct timeval */
#include <sys/types.h>          /* off_t */
#include <sys/wait.h>           /* waitpid(), WNOHANG */
#include <unistd.h>             /* access(), lseek(), {R,F}_OK */

#ifdef HAVE_PTRACE
#ifdef HAVE_SYS_PTRACE_H
#include <sys/ptrace.h>         /* ptrace(), PTRACE_* */
#endif /* HAVE_SYS_PTRACE_H */
#endif /* HAVE_PTRACE */

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>            /* statfs() */
#endif /* HAVE_SYS_VFS_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>          /* statfs() */
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif /* HAVE_SYS_MOUNT_H */

#ifdef HAVE_LINUX_MAGIC_H
#include <linux/magic.h>        /* PROC_SUPER_MAGIC */
#else
#ifndef PROC_SUPER_MAGIC
#define PROC_SUPER_MAGIC 0x9fa0
#endif /* PROC_SUPER_MAGIC */
#endif /* HAVE_LINUX_MAGIC_H */

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HAVE_PROCFS 
#ifndef PROCFS
#define PROCFS "/proc"
#endif /* PROCFS */
static bool check_procfs(pid_t);
#else
#error "some functions of libsandbox require procfs"
#endif /* HAVE_PROCFS */

typedef enum
{
    T_OPTION_NOP = 0, 
    T_OPTION_ACK = 1, 
    T_OPTION_END = 2, 
    T_OPTION_NEXT = 3, 
    T_OPTION_GETREGS = 4, 
    T_OPTION_GETDATA = 5, 
    T_OPTION_GETSIGINFO = 6, 
    T_OPTION_SETREGS = 7,
    T_OPTION_SETDATA = 8,
} option_t;

static long __trace(option_t, proc_t * const, void * const, long * const);

  
void *
sandbox_tracer(void * const dummy)
{
  
}


#include "sandbox.h"
#include "platform.h"
#include "internal.h" 
#include "config.h"

#include <errno.h>              /* ECHILD, EINVAL */
#include <grp.h>                /* struct group, getgrgid() */
#include <pwd.h>                /* struct passwd, getpwuid() */
#include <pthread.h>            /* pthread_{create,join,sigmask,...}() */
#include <signal.h>             /* kill(), SIG* */
#include <stdlib.h>             /* EXIT_{SUCCESS,FAILURE} */
#include <string.h>             /* str{cpy,cmp,str}(), mem{set,cpy}() */
#include <sys/stat.h>           /* struct stat, stat(), fstat() */
#include <sys/resource.h>       /* getrlimit(), setrlimit() */
#include <sys/wait.h>           /* waitid(), P_* */
#include <time.h>               /* clock_get{cpuclockid,time}(), ... */
#include <unistd.h>             /* fork(), access(), chroot(), getpid(),
                                   getpagesize(), {R,X}_OK,
                                   STD{IN,OUT,ERR}_FILENO */

#ifdef DELETED
#include <sys/time.h>           /* setitimer(), ITIMER_PROF */
#endif /* DELETED */

#ifdef WITH_REALTIME_SCHED
#warning "realtime scheduling is an experimental feature"
#ifdef HAVE_SCHED_H
#include <sched.h>              /* sched_set_scheduler(), SCHED_* */
#else /* NO_SCHED_H */
#error "<sched.h> is required by realtime scheduling but missing"
#endif /* HAVE_SCHED_H */
#endif /* WITH_REALTIME_SCHED */

#ifdef __cplusplus
extern "C"
{
#endif

/* Local macros (mostly for event handling) */

#define __QUEUE_EMPTY(pctrl) \
    (((pctrl)->event.size) <= 0) \
/* __QUEUE_EMPTY */

#define __QUEUE_FULL(pctrl) \
    (((pctrl)->event.size) >= (SBOX_EVENT_MAX)) \
/* __QUEUE_FULL */

#define __QUEUE_HEAD(pctrl) \
    ((pctrl)->event.list[((pctrl)->event.head)]) \
/* __QUEUE_HEAD */

#define __QUEUE_POP(pctrl) \
{{{ \
    if (!__QUEUE_EMPTY(pctrl)) \
    { \
        ++((pctrl)->event.head); \
        ((pctrl)->event.head) %= (SBOX_EVENT_MAX); \
        --((pctrl)->event.size); \
    } \
}}} /* __QUEUE_POP */

#define __QUEUE_PUSH(pctrl,elem) \
{{{ \
    assert(((pctrl)->event.size) < (SBOX_EVENT_MAX)); \
    (pctrl)->event.list[((((pctrl)->event.head) + \
        ((pctrl)->event.size)) % (SBOX_EVENT_MAX))] = (elem); \
    ++((pctrl)->event.size); \
}}} /* __QUEUE_PUSH */

#define __QUEUE_CLEAR(pctrl) \
{{{ \
    ((pctrl)->event.head) = ((pctrl)->event.size) = 0; \
    memset(((pctrl)->event.list), 0, (SBOX_EVENT_MAX) * sizeof(event_t)); \
}}} /* __QUEUE_CLEAR */

#define POST_EVENT(psbox,type,x...) \
{{{ \
    LOCK_ON_COND(psbox, EX, !__QUEUE_FULL(&((psbox)->ctrl))); \
    if (!HAS_RESULT(psbox)) \
    { \
        __QUEUE_PUSH(&((psbox)->ctrl), ((event_t){(S_EVENT ## type), {{x}}})); \
    } \
    UNLOCK(psbox); \
}}} /* POST_EVENT */

#define MONITOR_ERROR(psbox,x...) \
{{{ \
    WARN(x); \
    if (errno != ESRCH) \
    { \
        POST_EVENT((psbox), _ERROR, errno); \
        kill(-((psbox)->ctrl.pid), SIGKILL); \
    } \
}}} /* MONITOR_ERROR */

/* Macros for updating sandbox status / result */

#define __UPDATE_RESULT(psbox,res) \
{{{ \
    ((psbox)->result) = (result_t)(res); \
    DBUG("result: %s", s_result_name(((psbox)->result))); \
}}} /* __UPDATE_RESULT */

#define __UPDATE_STATUS(psbox,sta) \
{{{ \
    ((psbox)->status) = (status_t)(sta); \
    DBUG("status: %s", s_status_name(((psbox)->status))); \
}}} /* __UPDATE_STATUS */

#define UPDATE_RESULT(psbox,res) \
{{{ \
    LOCK(psbox, EX); \
    __UPDATE_RESULT(psbox, res); \
    UNLOCK(psbox); \
}}} /* UPDATE_RESULT */

#define UPDATE_STATUS(psbox,sta) \
{{{ \
    LOCK(psbox, EX); \
    __UPDATE_STATUS(psbox, sta); \
    UNLOCK(psbox); \
}}} /* UPDATE_STATUS */


  static void __sandbox_task_init(task_t *, const char * *);
static bool __sandbox_task_check(const task_t *);
static int  __sandbox_task_execute(task_t *);
static void __sandbox_task_fini(task_t *);

static void __sandbox_stat_init(stat_t *);
static void __sandbox_stat_update(sandbox_t *, const proc_t *);
static void __sandbox_stat_fini(stat_t *);

static void __sandbox_ctrl_init(ctrl_t *, thread_func_t);
static int  __sandbox_ctrl_add_monitor(ctrl_t *, thread_func_t);
static void __sandbox_ctrl_fini(ctrl_t *);

void * sandbox_watcher(sandbox_t *);
void * sandbox_profiler(sandbox_t *);

int 
sandbox_init(sandbox_t * psbox, const char * argv[])
{
  FUNC_BEGIN("%p,%p", psbox, argv);
  assert(psbox);
  
    if (psbox == NULL)
    {
        FUNC_RET("%d", -1);
    }
    
    psbox->lock = LOCK_INITIALIZER;
    LOCK(psbox, EX);

    __sandbox_task_init(&psbox->task, argv);
    __sandbox_stat_init(&psbox->stat);
    __sandbox_ctrl_init(&psbox->ctrl, (thread_func_t)sandbox_tracer);
    __sandbox_ctrl_add_monitor(&psbox->ctrl, (thread_func_t)sandbox_profiler);
    __sandbox_ctrl_add_monitor(&psbox->ctrl, (thread_func_t)sandbox_watcher);
    __UPDATE_RESULT(psbox, S_RESULT_PD);
    __UPDATE_STATUS(psbox, S_STATUS_PRE);
    UNLOCK(psbox);
    
    FUNC_RET("%d", 0);
}

static int
__sandbox_ctrl_add_monitor(ctrl_t * pctrl, thread_func_t tfm)
{
    FUNC_BEGIN("%p,%p", pctrl, tfm);
    assert(pctrl && tfm);
    
    int i;
    for (i = 0; i < (SBOX_MONITOR_MAX); i++)
    {
        if (pctrl->monitor[i].target == NULL)
        {
            pctrl->monitor[i].target = tfm;
            break;
        }
    }
    
    FUNC_RET("%d", i);
}
  

static void 
__sandbox_ctrl_init(ctrl_t * pctrl, thread_func_t tft)
{
    PROC_BEGIN("%p,%p", pctrl, tft);
    assert(pctrl && tft);
    
    memset(pctrl, 0, sizeof(ctrl_t));
    
    pctrl->policy.entry = (void *)sandbox_default_policy;
    pctrl->policy.data = 0L;
    memset(pctrl->monitor, 0, (SBOX_MONITOR_MAX) * sizeof(worker_t));
    memset(&pctrl->tracer, 0, sizeof(worker_t));
    pctrl->tracer.target = tft;
    __QUEUE_CLEAR(pctrl);
    
    PROC_END();
}  


static void 
__sandbox_stat_init(stat_t * pstat)
{
    PROC_BEGIN("%p", pstat);
    assert(pstat);
    
    memset(pstat, 0, sizeof(stat_t));
    
    PROC_END();
}

static void 
__sandbox_task_init(task_t * ptask, const char * argv[])
{
    PROC_BEGIN("%p,%p", ptask, argv);
    assert(ptask);              /* argv could be NULL */
    
    memset(ptask, 0, sizeof(task_t));
    
    unsigned int argc = 0;
    unsigned int offset = 0;
    if (argv != NULL)
    {
        while (argv[argc] != NULL)
        {
            unsigned int delta  = strlen(argv[argc]) + 1;
            if (offset + delta >= sizeof(ptask->comm.buff))
            {
                break;
            }
            strcpy(ptask->comm.buff + offset, argv[argc]);
            ptask->comm.args[argc++] = offset;
            offset += delta;
        }
    }
    ptask->comm.buff[offset] = '\0';
    ptask->comm.args[argc] = -1;
    
    strcpy(ptask->jail, "/");
    ptask->uid = getuid();
    ptask->gid = getgid();
    ptask->ifd = STDIN_FILENO;
    ptask->ofd = STDOUT_FILENO;
    ptask->efd = STDERR_FILENO;
    ptask->quota[S_QUOTA_WALLCLOCK] = SBOX_QUOTA_INF;
    ptask->quota[S_QUOTA_CPU] = SBOX_QUOTA_INF;
    ptask->quota[S_QUOTA_MEMORY] = SBOX_QUOTA_INF;
    ptask->quota[S_QUOTA_DISK] = SBOX_QUOTA_INF;
    PROC_END();
}


void *
sandbox_profiler(sandbox_t * psbox)
{
  
}  


void *
sandbox_watcher(sandbox_t * psbox)
{
  
}  

void 
sandbox_default_policy(const policy_t * ppolicy, const event_t * pevent, 
               action_t * paction)
{
  
}  

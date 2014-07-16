#define TCL_INTERPS             4

typedef int rivet_thr_status;

typedef struct {

    /* condition variable should be used with a mutex variable */

    apr_thread_mutex_t  *mutex;
    apr_thread_cond_t   *cond;
    apr_queue_t         *queue;
    apr_pool_t          *pool;
    Tcl_Interp*         interp_a[TCL_INTERPS];
    rivet_thr_status    status[TCL_INTERPS];
    int                 interp_idx;
    int                 busy_cnt;

} mod_rivet_globals;

typedef struct {
    request_rec         *r;
} rivet_request;

enum
{
    idle,
    request_processing
};

mod_rivet_globals module_globals;


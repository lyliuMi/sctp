#ifndef _L_FSM_H
#define _L_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

extern const char *FSM_NAME_INIT_SIG;
extern const char *FSM_NAME_ENTRY_SIG;
extern const char *FSM_NAME_EXIT_SIG;

typedef enum 
{
    FSM_ENTRY_SIG,
    FSM_EXIT_SIG,
    FSM_USER_SIG
}fsm_signal_e;

typedef void (*fsm_handler_t)(void *sm, void *event);

typedef struct _fsm_t 
{
    fsm_handler_t init;
    fsm_handler_t fini;
    fsm_handler_t state;
}fsm_t;

void fsm_init(void *fsm, void *init, void *fini, void *event);
void fsm_tran(void *fsm, void *state, void *event);
void fsm_dispatch(void *fsm, void *event);
void fsm_fini(void *fsm, void *event);

#define FSM_TRAN(__s, __target) \
    ((fsm_t *)__s)->state = (fsm_handler_t)(__target)

#define FSM_STATE(__s) \
    (((fsm_t *)__s)->state)

#define FSM_CHECK(__s, __f) \
    (FSM_STATE(__s) == (fsm_handler_t)__f)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

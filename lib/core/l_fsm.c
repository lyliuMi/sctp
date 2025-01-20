#include "l_fsm.h"
#include <assert.h>
#include <stdio.h>

typedef struct fsm_event_s 
{
    int id;
} fsm_event_t;

static fsm_event_t entry_event = {
    FSM_ENTRY_SIG,
};
static fsm_event_t exit_event = {
    FSM_EXIT_SIG,
};

const char *FSM_NAME_INIT_SIG = "INIT";
const char *FSM_NAME_ENTRY_SIG = "ENTRY";
const char *FSM_NAME_EXIT_SIG = "EXIT";

static void fsm_entry(fsm_t *sm, fsm_handler_t state, fsm_event_t *e)
{
    assert(sm);
    assert(state);

    if (e) {
        e->id = FSM_ENTRY_SIG;
        (*state)(sm, e);
    } else {
        (*state)(sm, &entry_event);
    }
}

static void fsm_exit(fsm_t *sm, fsm_handler_t state, fsm_event_t *e)
{
    assert(sm);
    assert(state);

    if (e) {
        e->id = FSM_EXIT_SIG;
        (*state)(sm, e);
    } else {
        (*state)(sm, &exit_event);
    }
}

static void fsm_change(
        fsm_t *sm,
        fsm_handler_t oldstate,
        fsm_handler_t newstate,
        fsm_event_t *e)
{
    assert(sm);
    assert(oldstate);
    assert(newstate);

    fsm_exit(sm, oldstate, e);
    fsm_entry(sm, newstate, e);
}

void fsm_init(void *fsm, void *init, void *fini, void *event)
{
    fsm_t *sm = fsm;
    fsm_event_t *e = event;

    assert(sm);

    sm->init = sm->state = init;
    sm->fini = fini;

    if (sm->init) {
        (*sm->init)(sm, e);

        if (sm->init != sm->state) {
            assert(sm->state);
            fsm_entry(sm, sm->state, e);
        }
    }
}

void fsm_tran(void *fsm, void *state, void *event)
{
    fsm_t *sm = fsm;
    fsm_event_t *e = event;
    fsm_handler_t tmp = NULL;

    assert(sm);

    tmp = sm->state;
    assert(tmp);

    sm->state = state;
    assert(sm->state);

    if (sm->state != tmp)
        fsm_change(fsm, tmp, sm->state, e);
}

void fsm_dispatch(void *fsm, void *event)
{
    fsm_t *sm = fsm;
    fsm_event_t *e = event;
    fsm_handler_t tmp = NULL;

    assert(sm);

    tmp = sm->state;
    assert(tmp);

    if (e)
        (*tmp)(sm, e);

    if (sm->state != tmp)
        fsm_change(fsm, tmp, sm->state, e);
}

void fsm_fini(void *fsm, void *event)
{
    fsm_t *sm = fsm;
    fsm_event_t *e = event;

    assert(sm);

    if (sm->fini != sm->state) {
        assert(sm->state);
        fsm_exit(sm, sm->state, e);

        if (sm->fini)
            (*sm->fini)(sm, e);
    }

    sm->init = sm->state = sm->fini = NULL;
}

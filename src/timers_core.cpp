// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

// debug facility
static debug_level_t uDebugLevel = DEBUG_WARN;

// timers data structures
static my_timer_t T[MAX_TIMERS];

void TIMER_ModInit(void) {
    _FOR(u, 0, MAX_TIMERS) {
        T[u].active = false;
        T[u].type = TIMER_SHOT_ONCE;
    }
}

static u8 timer_getFirstFreeTimer(void) {
    _FOR(u, 0, MAX_TIMERS)
        if (false == T[u].active)
            return u;

    return TIMER_NULL;
}

u8 TIMER_GetNumberOfFreeTimers(void) {
    u8 TimersAvail = 0;

    _FOR(u, 0, MAX_TIMERS)
        if (false == T[u].active)
            TimersAvail++;

    return TimersAvail;
}

bool TIMER_IsActive(u8 timer_id) {
    if (timer_id >= MAX_TIMERS)
        return false;

    return T[timer_id].active;
}

u8 TIMER_Start(const actions_t& i_rActions, actions_context_t& i_rActionContext,
    unsigned long time_seconds, timer_type_t i_eTimerType) {
    u8 iFreeTimer;

    // if free timer slot found, and not already ongoing ...
    if (TIMER_NULL != (iFreeTimer = timer_getFirstFreeTimer())) {
        T[iFreeTimer].m_actions = i_rActions;
        T[iFreeTimer].m_actions_context = i_rActionContext;
        T[iFreeTimer].time_start = now();
        T[iFreeTimer].time_stop = now() + time_seconds;
        T[iFreeTimer].active = true;
        T[iFreeTimer].type = i_eTimerType;

        // timer_id must be set here, since in "fun_start" there might be
        // already a reference to this field! Also, it's the only one
        // piece of information unknown till then ...
        T[iFreeTimer].m_actions_context.timer_id = iFreeTimer;

        IF_DEB_L() {
            String str(F("TIMER start: stop-start="));
            str += T[iFreeTimer].time_stop - T[iFreeTimer].time_start;
            str += F(", timer_id=");
            str += T[iFreeTimer].m_actions_context.timer_id;
            str += F(", slot=");
            str += T[iFreeTimer].m_actions_context.slot;
            str += F(", timer_id=");
            str += T[iFreeTimer].m_actions_context.timer_id;
            str += F(", active=");
            str += T[iFreeTimer].active;
            str += F(", type=");
            str += T[iFreeTimer].type;
            DEB_L(str);
        }

        // actual starting timer
        if (NULL != T[iFreeTimer].m_actions.fun_start)
            (T[iFreeTimer].m_actions.fun_start)(
                T[iFreeTimer].m_actions_context);
    };

    return iFreeTimer;
}

actions_context_t* TIMER_getActionContext(u8 timer_id) {
    if (TIMER_NULL == timer_id || timer_id >= MAX_TIMERS)
        return (NULL);

    return &T[timer_id].m_actions_context;
}

bool TIMER_ReStart(u8 timer_id, unsigned long time_seconds) {

    do {
        if (TIMER_NULL == timer_id || timer_id >= MAX_TIMERS)
            break;

        T[timer_id].time_start = now();
        T[timer_id].time_stop = now() + time_seconds;
        T[timer_id].active = true;

        return true;
    } while (0);

    DEB_E(F("ERR: TIMER_ReStart: stopping bad timer!\n"));
    THROW_ERROR();

    return false;
}

bool TIMER_Stop(u8 timer_id) {
    do {
        if (TIMER_NULL == timer_id || timer_id >= MAX_TIMERS)
            break;

        IF_DEB_L() {
            String str(F("TIMER: stop: timer_id="));
            str += timer_id;
            str += F(", var1=");
            str += T[timer_id].m_actions_context.var1;
            str += F(", var2=");
            str += T[timer_id].m_actions_context.var2;
            str += F(", slot=");
            str += T[timer_id].m_actions_context.slot;
            str += F(", timer_id=");
            str += T[timer_id].m_actions_context.timer_id;
            str += F(", active=");
            str += T[timer_id].active;
            str += F(", type=");
            str += T[timer_id].type;
            DEB_L(str);
        }

        if (false == T[timer_id].active)
            DEB_E(F("ERR: stopping already stopped timer!\n"));

        // first, internal structures clearing
        // that flag will change its state when user calls timer restart
        T[timer_id].active = false;

        // next, calling STOP function. This function can restart this timer!
        if (NULL != T[timer_id].m_actions.fun_stop) {
            (T[timer_id].m_actions.fun_stop)(T[timer_id].m_actions_context);

            // we called client's stop, but did he retriggered the timer?
            if (true == T[timer_id].active) { // we can expect deadline was set
                                              // by a client, so thats all
                IF_DEB_L() {
                    String str(F("TIMERS: client retriggered the timer"));
                    DEB_L(str);
                    MSG_Publish(MQTT_DEBUG, str.c_str());
                }
            }
        }
        else {
            IF_DEB_W() {
                String str(
                    F("TIMERS: calling stop on none existing timer function!"));
                DEB_W(str);
                MSG_Publish(MQTT_DEBUG, str.c_str());
            }
            THROW_ERROR();
            return false;
        }

        return true; // no problems
    } while (0);

    DEB_E(F("ERR: TIMER_Stop: stopping bad timer!\n"));
    THROW_ERROR();

    return false;
}

static void timer_TimerFunRecallStart(u8 i_TimerId) {
    if (NULL != T[i_TimerId].m_actions.fun_start)
        (T[i_TimerId].m_actions.fun_start)(T[i_TimerId].m_actions_context);
}

bool TIMER_ShutdownTimerSilently(u8 i_TimerId) {
    do {
        if (TIMER_NULL == i_TimerId || i_TimerId >= MAX_TIMERS)
            break;

        IF_DEB_L() {
            String str(F(" timer: shutting down: i_TimerId="));
            str += i_TimerId;
            str += F(", var1=");
            str += T[i_TimerId].m_actions_context.var1;
            str += F(", var2=");
            str += T[i_TimerId].m_actions_context.var2;
            str += F(", slot=");
            str += T[i_TimerId].m_actions_context.slot;
            str += F(", i_TimerId=");
            str += T[i_TimerId].m_actions_context.timer_id;
            str += F(", active=");
            str += T[i_TimerId].active;
            str += F(", type=");
            str += T[i_TimerId].type;
            DEB_L(str);
        }

        T[i_TimerId].active = false;

        return true;
    } while (0);

    DEB_E(F("ERR: timer_ShutdownTimerSilently: stopping bad timer!\n"));
    THROW_ERROR();

    return false;
}

void TIMER_ProcessAllTimers(void) {
    _FOR(i, 0, MAX_TIMERS)
        if (true == T[i].active) {
            // if a timer is expired, try to shut it down
            if (T[i].time_stop < now())
                TIMER_Stop(i);
            // else if timer is multi, just call it each sec
            else if (TIMER_SHOT_MULTIPLE == T[i].type)
                timer_TimerFunRecallStart(i);
        }
}

void TIMER_PrintActiveTimers(void) {
    bool bFirst = true;

    _FOR(i, 0, MAX_TIMERS)
        if (true == T[i].active) {
            if (true == bFirst) {
                String str1(F("\nActive timers:\n-=-=-=-=-="));
                // DEBLN(str1);
                MSG_Publish_Debug( str1.c_str());
                bFirst = false;
            }
            String str(F(" "));
            str += i;
            str += F(": var1=");
            str += T[i].m_actions_context.var1;
            str += F(",  var2=");
            str += T[i].m_actions_context.var2;
            str += F(",  slot=");
            str += T[i].m_actions_context.slot;
            str += F(",  now=");
            str += now();
            str += F(", Stop=");
            str += T[i].time_stop;
            str += F(" (sec=");
            str += T[i].time_stop - now();
            str += F(")");
            str += F(",  type=");
            str += T[i].type;
            DEB_T(str);
            MSG_Publish(MQTT_DEBUG, str.c_str());
        }
}

// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

// debug facility
static debug_level_t uDebugLevel = DEBUG_WARN;

// timers data structures
static my_timer_t T[MAX_TIMERS];

/// @brief Initializes the TIMER module
void TIMER_ModInit() {
    _FOR(u, 0, MAX_TIMERS) {
        T[u].active = false;
        T[u].type = TIMER_SHOT_ONCE;
    }
}

/// @brief Returns first free timer
/// @return returns timer id of the first free timer
static u8 timer_getFirstFreeTimer() {
    _FOR(u, 0, MAX_TIMERS)
        if (false == T[u].active)
            return u;

    return TIMER_NULL;
}

/// @brief Returns numer of free timers
/// @return number of free timers
u8 TIMER_GetNumberOfFreeTimers() {
    u8 TimersAvail = 0;

    _FOR(u, 0, MAX_TIMERS)
        if (false == T[u].active)
            TimersAvail++;

    return TimersAvail;
}

/// @brief Checks whether a timer is active
/// @param timer_id timer_id of timer to be checked
/// @return boolean value being true if timer is active
bool TIMER_IsActive(u8 timer_id) {
    if (timer_id >= MAX_TIMERS)
        return false;

    return T[timer_id].active;
}

/// @brief Starts a timer
/// @param i_rActions actions that should be triggered when timer starts and stops
/// @param i_rActionContext actions context (functions context)
/// @param time_seconds timer time span
/// @param i_eTimerType timer type (once vs multiple call)
/// @return timer_id of timer being allocated and launched. TIMER_NULL otherwise
u8 TIMER_Start(const actions_t& i_rActions, actions_context_t& i_rActionContext,
    unsigned long time_seconds, timer_type_t i_eTimerType) {
    u8 iFreeTimer;

    // if free timer slot found, and not already ongoing ...
    if (TIMER_NULL != (iFreeTimer = timer_getFirstFreeTimer())) {
        time_t _now= now();
        T[iFreeTimer].m_actions = i_rActions;
        T[iFreeTimer].m_actions_context = i_rActionContext;
        T[iFreeTimer].time_start = _now;
        T[iFreeTimer].time_stop = _now + time_seconds;
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
            str += F(", active=");
            str += T[iFreeTimer].active;
            str += F(", type=");
            str += T[iFreeTimer].type;
            DEB_L(str);
        }

        // actual starting timer
        if (NULL != T[iFreeTimer].m_actions.fun_start)
            (T[iFreeTimer].m_actions.fun_start)(T[iFreeTimer].m_actions_context);
    };

    return iFreeTimer;
}

/// @brief Retrieves function context of given timer
/// @param timer_id timer_id of which functions contenxt is being retrieved
/// @return pointer to the structure
actions_context_t* TIMER_getActionContext(u8 timer_id) {
    if (TIMER_NULL == timer_id || timer_id >= MAX_TIMERS)
        return NULL;

    return &T[timer_id].m_actions_context;
}

/// @brief Restarts given timer
/// @param timer_id timer_id which is going to be restarted
/// @param time_seconds next timer call period
/// @return result of the operation
bool TIMER_ReStart(u8 timer_id, unsigned long time_seconds) {

    do {
        if (TIMER_NULL == timer_id || timer_id >= MAX_TIMERS)
            break;

        T[timer_id].time_start = now();
        T[timer_id].time_stop = T[timer_id].time_start + time_seconds;
        T[timer_id].active = true;

        return true;
    } while (0);

    DEB_E(F("ERR: TIMER_ReStart: stopping bad timer!\n"));
    THROW_ERROR();

    return false;
}

/// @brief Stops given timer
/// @param timer_id timer_id which is going to be stopped
/// @return result of the operation
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
                // by a client, so this is all
                IF_DEB_L() {
                    String str(F("TIMERS: client retriggered the timer"));
                    DEB_L(str);
                    MSG_Publish_Debug(str.c_str());
                }
            }
        }
        else {
            IF_DEB_W() {
                String str(
                    F("TIMERS: calling stop on none existing timer"));
                DEB_W(str);
                MSG_Publish_Debug(str.c_str());
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

/// @brief helper local function to "start" given timer. Can be called multiple times for TIMER_SHOT_MULTIPLE timer type
/// @param timer_id timer_id which is going to be stopped
static void timer_TimerFunRecallStart(u8 i_TimerId) {
    if (NULL != T[i_TimerId].m_actions.fun_start)
        (T[i_TimerId].m_actions.fun_start)(T[i_TimerId].m_actions_context);
}

/// @brief Reset the timer of given id
/// @param i_TimerId id of a timer to be reset
/// @return result of the operation being true if successfully initialized
bool TIMER_ResetTimer(u8 i_TimerId) {
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

    DEB_E(F("ERR: TIMER_ResetTimer: stopping bad timer!\n"));
    THROW_ERROR();

    return false;
}

/// @brief Processes all timer, called once a second
/// @param  
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

/// @brief Pritins all active timers
/// @param  
void TIMER_PrintActiveTimers(void) {
    bool bFirst = true;

    _FOR(i, 0, MAX_TIMERS)
        if (true == T[i].active) {
            if (true == bFirst) {
                String str1(F("\nActive timers:\n-=-=-=-=-=-=-="));
                MSG_Publish_Debug(str1.c_str());
                bFirst = false;
            }
            time_t diff= T[i].time_stop - now();
            String str(F(" "));
            str += i;
            str += F(": var1=");
            str += T[i].m_actions_context.var1;
            str += F(", var2=");
            str += T[i].m_actions_context.var2;
            str += F(", slot=");
            str += T[i].m_actions_context.slot;
            str += F(", now=");
            str += now();
            str += F(", Stop=");
            str += T[i].time_stop;
            str += F(" (sec=");
            str += diff;
            str += F(")");
            str += F(", type=");
            str += T[i].type;
            str += F(", remaining=");
            time_t v= diff/SECS_IN_DAY;
            str += v; diff -= SECS_IN_DAY * v;
            str += F("d:");

            v= diff/SECS_IN_HOUR;
            str += v; diff -= SECS_IN_HOUR * v;
            str += F("h:");

            v= diff/SECS_IN_MINUTE;
            str += v; diff -= SECS_IN_MINUTE * v;
            str += F("m:");

            v= diff/SECS_IN_SEC;
            str += v; diff -= SECS_IN_SEC * v;
            str += F("s");

            //str += F("\n");
            DEB_T(str);
            MSG_Publish_Debug(str.c_str());
        }
}

// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"
#include "eeprom_wear.h"

#if 1 == N32_CFG_QUICK_ACTIONS_ENABLED

// Module name: Quick Actions
// Module aim: to quickly (locally) react to some input event like input value changed

static debug_level_t uDebugLevel = DEBUG_LOG;

#define QA_MAX_ENTRIES (5)

struct myEntry_s {
    triplet_t t;
    u8 src_cmnd_len; // we have triplets, this it's set to zero for now
    u8 dst_cmnd_len;
    u8 cmnd[MAX_QA_CMND_LENGTH];
};
struct myConfigData_s {
    u8 num_of_valid_entries;
    myEntry_s entries[QA_MAX_ENTRIES];
};

static struct myConfigData_s e2prom_cfg;
static bool isQAModulePaused = false;

static bool qa_isTripletCorrect(const triplet_t& t) {
    // TODO

    switch (t._topic)
    {
    case 'I':
        if (t._channel > 10)
            break; // false
        return true;

    default:
        break;
    };

    return false;
}

bool qa_decodeTiplet(triplet_t& t, const byte* payload) {
    t._topic = (*payload++);
    t._channel = (*payload++) - '0';
    t._value = (*payload++) - '0';

    return(qa_isTripletCorrect(t));
}


void QA_ModuleInit(void) {
    struct eeprom_wear_s<struct myConfigData_s> ee;

    if (false == ee.readCfgAbs(e2prom_cfg, 0)) {
        // nothing read
        e2prom_cfg.num_of_valid_entries = 0;
    }

    // Info display
    IF_DEB_L() {
        String str(F("QA: init with: "));
        str += e2prom_cfg.num_of_valid_entries;
        str += F(" quick actions.");
        SERIAL_publish(str.c_str());
    }
}

bool QA_isStateTracked(char i_Modules, u8 i_Channel, u8 i_uState, u8& o_uSlotNumber) {
    _FOR(i, 0, e2prom_cfg.num_of_valid_entries) {
        triplet_t& t = e2prom_cfg.entries[i].t;

        if (t._topic == i_Modules && t._channel == i_Channel && t._value == i_uState) {
            IF_DEB_L() {
                String str(F("QA: found slot: "));
                str += i;
                SERIAL_publish(str.c_str());
            }
            o_uSlotNumber = i;
            return true;
        }

        IF_DEB_L() {
            String str(F("QA: Flow not matched due to: t._topic="));
            str += t._topic;
            str += F(", i_Modules=");
            str += i_Modules;
            str += F(", t._channel=");
            str += t._channel;
            str += F(", i_Channel=");
            str += i_Channel;
            str += F(", t._value=");
            str += t._value;
            str += F(", i_uState=");
            str += i_uState;
            SERIAL_publish(str.c_str());
        }

    }
    return false;
}

bool QA_ExecuteCommand(u8 slot_num) {
    struct myEntry_s& e = e2prom_cfg.entries[slot_num];

    IF_DEB_T() {
        String str(F("QA: Executing command for slot: "));
        str += slot_num;
        SERIAL_publish(str.c_str());
    }

    return(CMNDS_Launch(&e.cmnd[e.src_cmnd_len]));
}

// Q0 I111 L201M1 1
static bool qa_StoreConfiguration(const byte* command, const triplet_t& i_t, u8 i_src_cmndLen, u8 i_dst_cmndLen) {
    struct myConfigWrapper_s<myConfigData_s> e2prom_cfg_wrapper = myConfigWrapper_s<myConfigData_s>(e2prom_cfg);
    struct eeprom_wear_s<struct myConfigData_s> ee;

    // we can assume entries is valid here, since this module has been already initialized
    u8 count = e2prom_cfg.num_of_valid_entries;
    if (count < QA_MAX_ENTRIES) {
        command += i_src_cmndLen;// we're skipping source command since this data is kep in the triplet
        memset(&(e2prom_cfg.entries[count].cmnd), 0, MAX_QA_CMND_LENGTH); // clearing
        memcpy(&(e2prom_cfg.entries[count].cmnd), command, i_dst_cmndLen); // copying only executable command, source remains as a triplet

        e2prom_cfg.entries[count].t = i_t;
        e2prom_cfg.entries[count].src_cmnd_len = 0; // for now we're using triples for source commands identification
        e2prom_cfg.entries[count].dst_cmnd_len = i_dst_cmndLen;
        e2prom_cfg.num_of_valid_entries++;

        ee.writeCfgAbs(e2prom_cfg, 0);

        IF_DEB_L() {
            String str(F("QA: saved new slot to eeprom: count="));
            str += e2prom_cfg_wrapper.data.num_of_valid_entries;
            str += F(", cmnd len=");
            str += i_src_cmndLen + i_dst_cmndLen;
            str += F(", src_cmndLen=");
            str += i_src_cmndLen;
            str += F(", dst_cmndLen=");
            str += i_dst_cmndLen;
            SERIAL_publish(str.c_str());
        }

        return true;
    }
    else {
        IF_DEB_W() {
            String str(F("QA: no more free entries available!"));
            SERIAL_publish(str.c_str());
        }
    }

    return false;
}

// Q0 I111 L201M1 1
static bool qa_AnalyzeCommand(const byte* command, triplet_t& o_t, u8& o_src_cmndLen, u8& o_dst_cmndLen) {
    //decodedLen = 0;

    if (false == qa_decodeTiplet(o_t, command)) {
        IF_DEB_L() {
            String str(F("QA: bad tripplet: '"));
            str += command[0];
            str += F(", ");
            str += command[1];
            str += F(", ");
            str += command[2];
            SERIAL_publish(str.c_str());
        }
        return false;
    }

    state_t s; // value stored here will be ignored
    o_src_cmndLen = sizeof(triplet_t) + 1; // 3 bytes for triplet + 1 byte of checksum
    o_dst_cmndLen = 0;

    command += o_src_cmndLen; // for now source command is stored as a triplet, so skipping

    // let's try to decode destination command
    if (false == CMNDS_decodeCmnd(command, s, &o_dst_cmndLen)) {
        IF_DEB_L() {
            String str(F("QA: bad decoding with a command"));
            SERIAL_publish(str.c_str());
        }
        return false;
    }
    o_dst_cmndLen += 1; // TODO: why is this needed?

    IF_DEB_T() {
        String str(F("QA: decoded dest length: '"));
        str += o_dst_cmndLen;
        str += F("'");
        SERIAL_publish(str.c_str());
    }

    return true; // error
}

static bool qa_RemoveCommand(const byte* command, u8& decodedLen) {
    triplet_t t;

    if (false == qa_decodeTiplet(t, command)) {
        IF_DEB_L() {
            String str(F("QA: bad tripplet: '"));
            str += command[0];
            str += F(", ");
            str += command[1];
            str += F(", ");
            str += command[2];
            str += F(", ");
            str += command[3];
            SERIAL_publish(str.c_str());
        }
        return false;
    }

    u8 slotNumber;
    if (true == QA_isStateTracked(t._topic, t._channel, t._value, slotNumber)) {
        struct eeprom_wear_s<struct myConfigData_s> ee;

        e2prom_cfg.entries[slotNumber] = e2prom_cfg.entries[e2prom_cfg.num_of_valid_entries];

        --e2prom_cfg.num_of_valid_entries;

        // final write
        ee.writeCfgAbs(e2prom_cfg, 0);

        return true;
    }

    return false; // no cmnd removed, so no success in removing
}

static bool qa_RemoveAll() {
    struct eeprom_wear_s<struct myConfigData_s> ee;

    if (0x0 == e2prom_cfg.num_of_valid_entries) {
        IF_DEB_L() {
            String str(F("QA: clearing already cleared QA commands"));
            SERIAL_publish(str.c_str());
        }
    }
    e2prom_cfg.num_of_valid_entries = 0;

    // final write
    ee.writeCfgAbs(e2prom_cfg, 0);

    return true;
}


void QA_DisplayAssignments(void) {
    {
        String str(F("\nQuick Actions registered:\n-=-=-=-=-="));
        MSG_Publish_Debug(str.c_str());
    }

    u8 SlotsRegistered = 0;
    _FOR(i, 0, e2prom_cfg.num_of_valid_entries) {
        struct myEntry_s& e = e2prom_cfg.entries[i];

        String str = F("");
        str += i;
        str += F(": S=");
        str += e.src_cmnd_len;
        str += F(": D=");
        str += e.dst_cmnd_len;
        str += F(": Src='");
        _FOR(j, 0, e.src_cmnd_len) {
            str += (char)e.cmnd[j];
            //DEB((char)e.cmnd[j]);
        }
        str += F("', Dst: '");
        _FOR(k, 0, e.dst_cmnd_len) {
            str += (char)e.cmnd[k];
            //DEB((char)e.cmnd[k]);
        }
        str += F("'");
        MSG_Publish_Debug(str.c_str());
        SlotsRegistered++;
    };
    String str = F("Total flows registered: ");
    str += SlotsRegistered;
    str += F("\n");
    MSG_Publish_Debug(str.c_str());
}

/**
 * Q0TCV0123456789 - Register a new quick command in topic "T", channel "C", value "V",
 *                      so the command "0123456789" is sent. "n" has max length of 10 bytes
 * Q1TCV0123456789 - Remove given quick action
 * Q2         - Clear all quick actions
 * Q3         - Disable all quick actions
 * Q4         - Enable all quick actions
 */
bool decode_CMND_Q(const byte* payload, state_t& s, u8* o_CmndLen) {
    const byte* cmndStart = payload;

    // DEB("\n----------------- in decoder:\n");
    // _FOR(i, 0, 10)
    //     DEB((char)payload[i]);

    u8 src_cmndLen, dst_cmndLen;
    triplet_t t;

    s.command = (*payload++) - '0'; // 0..4
    bool sanity_ok = false;

    switch (s.command) {
    case CMND_QA_REGISTER_NEW_CMND:
    case CMND_QA_REMOVE_CMND:
        sanity_ok = qa_AnalyzeCommand(payload, t, src_cmndLen, dst_cmndLen);
        break;

    case CMND_QA_CLEAR_ALL_CMNDS:
    case CMND_QA_DISABLE_ALL_CMNDS:
    case CMND_QA_ENABLE_ALL_CMNDS:
        sanity_ok = true;
        break;

    default:
        break;
    }

    u8 decodedCmndLen = 0;
    //// Q0 I111 L201M1 1
    if (true == sanity_ok) {

        // DEB("----------------- in decoder2:\n");
        // _FOR(i, 0, 15)
        //     DEB((char)payload[i]);

        const byte* cmnd_base = payload;

        decodedCmndLen = src_cmndLen + dst_cmndLen;
        payload += decodedCmndLen;

        // DEB("\n----------------- in decoder3:\n");
        // _FOR(i, 0, 15)
        //     DEB((char)payload[i]);

        s.sum = (*payload++) - '0'; // sum = 1

        String str(F("\nQA: s.sum= "));
        str += s.sum;
        str += F(", payload -1: '");
        str += char(*(payload - 1));
        str += F(", payload: '");
        str += char(*(payload));
        str += F("', decodedCmndLen: ");
        str += decodedCmndLen;
        SERIAL_publish(str.c_str());

        if (true == (sanity_ok = isSumOk(s))) {
            // it's ok, so change the states
            switch (s.command) {
            case CMND_QA_REGISTER_NEW_CMND:
                sanity_ok = qa_StoreConfiguration(cmnd_base, t, src_cmndLen, dst_cmndLen);
                break;

            case CMND_QA_REMOVE_CMND:
                sanity_ok = qa_RemoveCommand(cmnd_base, decodedCmndLen);
                break;

            case CMND_QA_CLEAR_ALL_CMNDS:
                sanity_ok = qa_RemoveAll();
                break;

            case CMND_QA_DISABLE_ALL_CMNDS:
                isQAModulePaused = true;
                break;

            case CMND_QA_ENABLE_ALL_CMNDS:
                isQAModulePaused = false;
                break;

            default:
                break;
            }
        }
    }

    // Info display
    IF_DEB_L() {
        String str(F("QA: Cmd: "));
        str += s.command;
        str += F(", Sanity: ");
        str += sanity_ok;
        str += F(", decoded cmnd len: ");
        str += decodedCmndLen;
        str += F(", Sum: ");
        str += (s.sum + '0');
        SERIAL_publish(str.c_str());
    }

    // setting decoded and valid cmnd length
    if (NULL != o_CmndLen)
        *o_CmndLen = payload - cmndStart;

    return (sanity_ok);
}

module_caps_t QA_getCapabilities(void) {
    module_caps_t mc = {
        .m_is_input = false,
        .m_number_of_channels = QUICKACTIONS_NUM_OF_AVAIL_CHANNELS,
        .m_module_name = F("QA"),
        .m_mod_init = QA_ModuleInit,
        .m_cmnd_decoder = decode_CMND_Q,
        .m_cmnd_executor = 0
    };

    return(mc);
}
#endif // N32_CFG_QUICK_ACTIONS_ENABLED

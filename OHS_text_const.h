/*
 * OHS_text_const.h
 *
 *  Created on: Dec 8, 2020
 *      Author: vysocan
 */

#ifndef OHS_TEXT_CONST_H_
#define OHS_TEXT_CONST_H_

const char weekNumber[][7] = {
// 12345678901234567890
  "Last",
  "First",
  "Second",
  "Third",
  "Fourth"
};

const char weekDay[][10] = {
// 12345678901234567890
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday",
  "Sunday"
};

const char weekDayShort[][3] = {
  "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"
};

const char monthName[][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char durationSelect[][10] = {
// 12345678901234567890
  "second(s)",
  "minute(s)",
  "hour(s)",
  "day(s)"
};

const char text_System[]            = "System";
const char text_info[]              = "information";
const char text_started[]           = "started";
const char text_Undefined[]         = "Undefined";
const char text_removed[]           = "removed";
const char text_disabled[]          = "disabled";
const char text_address[]           = "address";
const char text_unable[]            = "unable";
const char text_to[]                = "to";
const char text_resolve[]           = "resolve";
const char text_Address[]           = "Address";
const char text_Group[]             = "Group";
const char text_group[]             = "group";
const char text_registration[]      = "registration";
const char text_error[]             = "error";
const char text_failure[]           = "failure";
const char text_registered[]        = "registered";
const char text_is[]                = "is";
const char text_Authentication[]    = "Authentication";
const char text_authentication[]    = "authentication";
const char text_disarming[]         = "disarming";
const char text_Sensor[]            = "Sensor";
const char text_sensor[]            = "sensor";
const char text_Input[]             = "Output"; // :) Input on node is Output to GW
const char text_Output[]            = "Output"; // :) Input on node is Output to GW
const char text_iButton[]           = "iButton";
const char text_Temperature[]       = "Temperature";
const char text_Humidity[]          = "Humidity";
const char text_Pressure[]          = "Pressure";
const char text_Voltage[]           = "Voltage";
const char text_voltage[]           = "voltage";
const char text_Battery[]           = "Battery";
const char text_RTC[]               = "RTC";
const char text_Digital[]           = "Digital";
const char text_Analog[]            = "Analog";
const char text_Float[]             = "Float";
const char text_TX_Power[]          = "TX_Power";
const char text_Gas[]               = "Gas";
const char text_not[]               = "not";
const char text_authorized[]        = "authorized";
const char text_disconnected[]      = "disconnected";
const char text_connect[]           = "connect";
const char text_timeout[]           = "timeout";
const char text_refused[]           = "refused";
const char text_server[]            = "server";
const char text_protocol[]          = "protocol";
const char text_version[]           = "version";
const char text_identifier[]        = "identifier";
const char text_strength[]          = "strength";
const char text_unknown[]           = "unknown";
const char text_empty[]             = "empty";
const char text_Empty[]             = "Empty";
const char text_roaming[]           = "roaming";
const char text_searching[]         = "searching";
const char text_network[]           = "network";
const char text_denied[]            = "denied";
const char text_cosp[]              = ", ";
const char text_Modem[]             = "Modem";
const char text_On[]                = "On";
const char text_Off[]               = "Off";
const char text_1[]                 = "1";
const char text_2[]                 = "2";
const char text_3[]                 = "3";
const char text_4[]                 = "4";
const char text_0x[]                = "0x";
const char text_1x[]                = "1x";
const char text_2x[]                = "2x";
const char text_3x[]                = "3x";
const char text_power[]             = "power";
const char text_main[]              = "main";
const char text_monitoring[]        = "monitoring";
const char text_Node[]              = "Node";
const char text_Name[]              = "Name";
const char text_name[]              = "name";
const char text_MQTT[]              = "MQTT";
const char text_Function[]          = "Function";
const char text_Type[]              = "Type";
const char text_type[]              = "type";
const char text_publish[]           = "publish";
const char text_subscribe[]         = "subscribe";
const char text_Subscribe[]         = "Subscribe";
const char text_Last[]              = "Last";
const char text_devation[]          = "deviation";
const char text_message[]           = "message";
const char text_Queued[]            = "Queued";
const char text_Value[]             = "Value";
const char text_Number[]            = "Number";
const char text_Email[]             = "Email";
const char text_Global[]            = "Global";
const char text_Contact[]           = "Contact";
const char text_User[]              = "User";
const char text_all[]               = "all";
const char text_Key[]               = "Key";
const char text_Open[]              = "Open";
const char text_alarm[]             = "alarm";
const char text_Alarm[]             = "Alarm";
const char text_as[]                = "as";
const char text_tamper[]            = "tamper";
const char text_Delay[]             = "Delay";
const char text_OK[]                = "OK";
const char text_Status[]            = "Status";
const char text_remote[]            = "remote";
const char text_local[]             = "local";
const char text_battery[]           = "battery";
const char text_analog[]            = "analog";
const char text_digital[]           = "digital";
const char text_Zone[]              = "Zone";
const char text_zone[]              = "zone";
const char text_delay[]             = "delay";
const char text_SMS[]               = "SMS";
const char text_Page[]              = "Page";
const char text_disarmed[]          = "disarmed";
const char text_pending[]           = "pending";
const char text_armed[]             = "armed";
const char text_auto[]              = "auto";
const char text_open[]              = "open";
const char text_allowed[]           = "allowed";
const char text_matched[]           = "matched";
const char text_re[]                = "re";
const char text_Date[]              = "Date";
const char text_Entry[]             = "Entry";
const char text_Alert[]             = "Alert";
const char text_key[]               = "key";
const char text_value[]             = "value";
const char text_once[]              = "once";
const char text_after[]             = "after";
const char text_always[]            = "always";
const char text_constant[]          = "constant";
const char text_connected[]         = "connected";
const char text_Connected[]         = "Connected";
const char text_Armed[]             = "Armed";
const char text_Arm[]               = "Arm";
const char text_arm[]               = "arm";
const char text_arm_home[]          = "arm_home";
const char text_arm_away[]          = "arm_away";
const char text_disarm[]            = "disarm";
const char text_arming[]            = "arming";
const char text_Disarm[]            = "Disarm";
const char text_chain[]             = "chain";
const char text_trigger[]           = "trigger";
const char text_triggered[]         = "triggered";
const char text_Trigger[]           = "Trigger";
const char text_Auto[]              = "Auto";
const char text_Tamper[]            = "Tamper";
const char text_relay[]             = "relay";
const char text_home[]              = "home";
const char text_away[]              = "away";
const char text_Time[]              = "Time";
const char text_time[]              = "time";
const char text_Start[]             = "Start";
const char text_Up[]                = "Up";
const char text_AC[]                = "AC";
const char text_Register[]          = "Register";
const char text_Signal[]            = "Signal";
const char text_Alive[]             = "Alive";
const char text_Admin[]             = "Admin";
const char text_user[]              = "user";
const char text_Password[]          = "Password";
const char text_password[]          = "password";
const char text_SMTP[]              = "SMTP";
const char text_NTP[]               = "NTP";
const char text_Radio[]             = "Radio";
const char text_Frequency[]         = "Frequency";
const char text_Server[]            = "Server";
const char text_port[]              = "port";
const char text_of[]                = "of";
const char text_at[]                = "at";
const char text_offset[]            = "offset";
const char text_end[]               = "end";
const char text_start[]             = "start";
const char text_DS[]                = "Daylight saving";
const char text_Standard[]          = "Standard";
const char text_format[]            = "format";
const char text_oclock[]            = "o'clock";
const char text_Balanced[]          = "Balanced";
const char text_balanced[]          = "balanced";
const char text_low[]               = "low";
const char text_state[]             = "state";
const char text_Blocks[]            = "Blocks";
const char text_Entries[]           = "Entries";
const char text_Used[]              = "Used";
const char text_Free[]              = "Free";
const char text_Total[]             = "Total";
const char text_Metric[]            = "Metric";
const char text_Hash[]              = "Hash";
const char text_Period[]            = "Period";
const char text_Run[]               = "Run";
const char text_Script[]            = "Script";
const char text_Next[]              = "Next";
const char text_on[]                = "on";
const char text_off[]               = "off";
const char text_Calendar[]          = "Calendar";
const char text_Duration[]          = "Duration";
const char text_duration[]          = "duration";
const char text_Timer[]             = "Timer";
const char text_timer[]             = "timer";
const char text_kB[]                = "kB";
const char text_Heap[]              = "Heap";
const char text_heap[]              = "heap";
const char text_Storage[]           = "Storage";
const char text_storage[]           = "storage";
const char text_Fragmentation[]     = "Fragmentation";
const char text_Evaluate[]          = "Evaluate";
const char text_script[]            = "script";
const char text_Result[]            = "Result";
const char text_linked_to[]         = "linked to";
const char text_Condition[]         = "Condition";
const char text_Hysteresis[]        = "Hysteresis";
const char text_Pass[]              = "Pass";
const char text_To[]                = "To";
const char text_queue[]             = "queue";
const char text_full[]              = "full";
const char text_Registration[]      = "Registration";
const char text_not_found[]         = "not found";
const char text_activated[]         = "activated";
const char text_error_free[]        = "Not enough free space in ";

#endif /* OHS_TEXT_CONST_H_ */

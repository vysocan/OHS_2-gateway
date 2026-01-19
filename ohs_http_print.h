/*
 * ohs_http_print.h
 *
 *  Created on: Apr 28, 2021
 *      Author: vysocan
 */

#ifndef OHS_HTTP_PRINT_H_
#define OHS_HTTP_PRINT_H_
// Helper macro
#define GET_BUTTON_STATE(x,y) (x==y)
/*
 *
 */
void printOkNok(BaseSequentialStream *chp, const int8_t value) {
  if (value == 1) chprintf(chp, "%s", HTML_i_OK);
  else            chprintf(chp, "%s", HTML_i_disabled);
}
/*
 *
 */
void printRadioButton(BaseSequentialStream *chp, const char *name, const uint8_t value,
                 const char *label, bool selected,
                 const uint8_t JSNumber, const uint8_t JSMask) {
  chprintf(chp, "%s%s%s", HTML_cbPart1a, name, HTML_cbPart1b);
  chprintf(chp, "%s%u%s%u", name, value, HTML_cbPart2, value);
  selected ? chprintf(chp, "%s", HTML_cbChecked) : chprintf(chp, "%s", HTML_cbPart3);
  if (JSNumber) {
    if ((JSMask >> value) & 0b1) {
      chprintf(chp, "%s%u%s", HTML_cbJSen, JSNumber, HTML_cbJSend);
    } else {
      chprintf(chp, "%s%u%s", HTML_cbJSdis, JSNumber, HTML_cbJSend);
    }
  }
  chprintf(chp, "%s%s%u", HTML_cbPart4a, name, value);
  chprintf(chp, "%s%s%s", HTML_cbPart4b, label, HTML_cbPart5);
}
/*
 *
 */
void printTwoButton(BaseSequentialStream *chp, const char *name, const uint8_t state,
                    const uint8_t JSNumber, const uint8_t JSMask,
                     const char *text1, const char *text2) {
  chprintf(chp, "%s", HTML_radio_sl);
  printRadioButton(chp, name, 0, text1, GET_BUTTON_STATE(state, 0), JSNumber, JSMask);
  printRadioButton(chp, name, 1, text2, GET_BUTTON_STATE(state, 1), JSNumber, JSMask);
  chprintf(chp, "%s", HTML_div_e);
}
/*
 *
 */
// ((JSon >> 1) & 0b1) : enableJS
void printThreeButton(BaseSequentialStream *chp, const char *name, const uint8_t state,
                      const uint8_t JSNumber, const uint8_t JSMask,
                      const char *text1, const char *text2, const char *text3,
                      const uint8_t size) {
  if (size) chprintf(chp, "%s", HTML_radio_sb);
  else      chprintf(chp, "%s", HTML_radio_sl);
  printRadioButton(chp, name, 0, text1, GET_BUTTON_STATE(state, 0), JSNumber, JSMask);
  printRadioButton(chp, name, 1, text2, GET_BUTTON_STATE(state, 1), JSNumber, JSMask);
  printRadioButton(chp, name, 2, text3, GET_BUTTON_STATE(state, 2), JSNumber, JSMask);
  chprintf(chp, "%s", HTML_div_e);
}
/*
 *
 */
void printFourButton(BaseSequentialStream *chp, const char *name, const uint8_t state,
                     const uint8_t JSNumber, const uint8_t JSMask,
                     const char *text1, const char *text2, const char *text3,
                     const char *text4, const uint8_t size) {
  if (size) chprintf(chp, "%s", HTML_radio_sb);
  else      chprintf(chp, "%s", HTML_radio_sl);
  printRadioButton(chp, name, 0, text1, GET_BUTTON_STATE(state, 0), JSNumber, JSMask);
  printRadioButton(chp, name, 1, text2, GET_BUTTON_STATE(state, 1), JSNumber, JSMask);
  printRadioButton(chp, name, 2, text3, GET_BUTTON_STATE(state, 2), JSNumber, JSMask);
  printRadioButton(chp, name, 3, text4, GET_BUTTON_STATE(state, 3), JSNumber, JSMask);
  chprintf(chp, "%s", HTML_div_e);
}
/*
 *
 */
void printOnOffButton(BaseSequentialStream *chp, const char *name, const uint8_t state) {
  chprintf(chp, "%s", HTML_radio_s);
  printRadioButton(chp, name, 1, TEXT_On, state, 0, 0);
  printRadioButton(chp, name, 0, TEXT_Off, !state, 0, 0);
  chprintf(chp, "%s", HTML_div_e);
}
/*
 *
 */
void printOnOffButtonWJS(BaseSequentialStream *chp, const char *name, const uint8_t state,
                         const uint8_t JSNumber, const uint8_t JSMask) {
  chprintf(chp, "%s", HTML_radio_s);
  printRadioButton(chp, name, 1, TEXT_On, state, JSNumber, JSMask);
  printRadioButton(chp, name, 0, TEXT_Off, !state, JSNumber, JSMask);
  chprintf(chp, "%s", HTML_div_e);
}
/*
 *
 */
void selectGroup(BaseSequentialStream *chp, uint8_t selected, char name) {
  chprintf(chp, "%s%c%s%c%s", HTML_select, name, HTML_id_tag, name, HTML_e_tag);
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (selected == i) { chprintf(chp, "%s", HTML_selected); }
    else               { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%u. %s - ", i + 1, conf.group[i].name);
    GET_CONF_GROUP_ENABLED(conf.group[i].setting) ? chprintf(chp, "%s", TEXT_On) : chprintf(chp, "%s", TEXT_Off);
    chprintf(chp, "%s", HTML_e_option);
  }
  chprintf(chp, "%s%u", HTML_option, DUMMY_GROUP);
  if (selected == DUMMY_GROUP) { chprintf(chp, "%s", HTML_selected); }
  else                         { chprintf(chp, "%s", HTML_e_tag); }
  chprintf(chp, "%s%s", NOT_SET, HTML_e_option);
  chprintf(chp, "%s", HTML_e_select);
}
/*
 * Print node value to stream
 */
void printNodeValue(BaseSequentialStream *chp, const uint8_t index) {
  switch(node[index].function){
    case 'T': chprintf(chp, "%.2f °C", node[index].value); break;
    case 'H':
    case 'X': chprintf(chp, "%.2f %%", node[index].value); break;
    case 'P': chprintf(chp, "%.2f mbar", node[index].value); break;
    case 'V':
    case 'B': chprintf(chp, "%.2f V", node[index].value); break;
    case 'G': chprintf(chp, "%.2f ppm", node[index].value); break;
    case 'I': chprintf(chp, "%.0f lux", node[index].value); break;
    case 'i':
      // Check if key index is valid
      if ((uint8_t)node[index].value < KEYS_SIZE) {
        chprintf(chp, "%s", conf.contact[conf.key[(uint8_t)node[index].value].contact].name);
      } else {
        chprintf(chp, "%s", TEXT_unknown);
      }
      break;
    default: chprintf(chp, "%.2f", node[index].value); break;
  }
}
/*
 *
 */
void printTextInput(BaseSequentialStream *chp, const char name, const char *value,
                    const uint8_t size){
  chprintf(chp, "%s%u%s%u%s", HTML_t_tag_1, size - 1, HTML_s_tag_2, size - 1, HTML_s_tag_3);
  chprintf(chp, "%c%s%s", name, HTML_m_tag, value);
  chprintf(chp, "%s%c%s", HTML_id_tag, name, HTML_e_tag);
}
/*
 *
 */
void printTextInputWMin(BaseSequentialStream *chp, const char name, const char *value,
                        const uint8_t size, const uint8_t minSize){
  chprintf(chp, "%s%u%s%u", HTML_t_tag_1, size, HTML_s_tag_2, size);
  chprintf(chp, "%s%u%s", HTML_s_tag_4, minSize, HTML_s_tag_3);
  chprintf(chp, "%c%s%s", name, HTML_m_tag, value);
  chprintf(chp, "%s%c%s", HTML_id_tag, name, HTML_e_tag);
}
/*
 *
 */
void printPassInput(BaseSequentialStream *chp, const char name, const char *value,
                    const uint8_t size, const uint8_t minSize){
  chprintf(chp, "%s%u%s%u", HTML_p_tag_1, size - 1, HTML_s_tag_2, size - 1);
  chprintf(chp, "%s%u%s", HTML_s_tag_4, minSize, HTML_s_tag_3);
  chprintf(chp, "%c%s%s", name, HTML_m_tag, value);
  chprintf(chp, "%s%c%s", HTML_id_tag, name, HTML_e_tag);
}
/*
 *
 */
void printIntInput(BaseSequentialStream *chp, const char name, const int16_t value,
                   const uint8_t size, const int32_t min, const int32_t max){
  chprintf(chp, "%s%u", HTML_n_tag_1, size + 2);
  chprintf(chp, "%s%d%s%d%s", HTML_n_tag_2, min, HTML_n_tag_3, max, HTML_s_tag_3);
  chprintf(chp, "%c%s%d", name, HTML_m_tag, value);
  chprintf(chp, "%s%c%s", HTML_id_tag, name, HTML_e_tag);
}
/*
 *
 */
void printFloatInput(BaseSequentialStream *chp, const char name, const float value){
  chprintf(chp, "%s6em%s", HTML_n_tag_1, HTML_s_tag_3);
  chprintf(chp, "%c%s%.02f", name, HTML_m_tag, value);
  chprintf(chp, "%s%c' step='0.01'>", HTML_id_tag, name);
}
/*
 *
 */
void printTimeInput(BaseSequentialStream *chp, const char name, const uint8_t hour,
                    const uint8_t minute){
  chprintf(chp, "%s%s%c", HTML_i_tag_1, HTML_s_tag_3, name);
  chprintf(chp, "%s%c%s", HTML_id_tag, name, HTML_m_tag);
  chprintf(chp, "%02u:%02u%s", hour, minute, HTML_i_tag_2);
}
// IPv4: <input type="text" minlength="7" maxlength="15" size="15" pattern="^((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])(\.(?!$)|$)){4}$">
/*
 *
 */
void printTextArea(BaseSequentialStream *chp, const char name, const char *value,
                   const uint16_t maxSize, const uint8_t cols, const uint8_t rows){
  chprintf(chp, "%s%c%s%c%s%u", HTML_textarea_1, name, HTML_textarea_2, name, HTML_textarea_3, rows);
  chprintf(chp, "%s%u%s%u' class='input' spellcheck='false'>", HTML_textarea_4, cols, HTML_textarea_5, maxSize - 1);
  chprintf(chp, "%s%s", value, HTML_textarea_e);
}
/*
 *
 */
void printDurationSelect(BaseSequentialStream *chp, const char name, const uint8_t value){
  chprintf(chp, "%s%c%s%c%s", HTML_select, name, HTML_id_tag, name, HTML_e_tag);
   for (uint8_t i = 0; i < ARRAY_SIZE(durationSelect); i++) {
     chprintf(chp, "%s%u", HTML_option, i);
     if (value == i) { chprintf(chp, "%s", HTML_selected); }
     else            { chprintf(chp, "%s", HTML_e_tag); }
     chprintf(chp, "%s%s", durationSelect[i], HTML_e_option);
   }
   chprintf(chp, "%s", HTML_e_select);
}

#endif /* OHS_HTTP_PRINT_H_ */

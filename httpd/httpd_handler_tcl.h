/*
 * httpd_handler_tcl.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_TCL_H_
#define HTTPD_HANDLER_TCL_H_


/*
 * @brief HTTP tcl page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_tcl(BaseSequentialStream *chp) {
  uint16_t logAddress;
  // Information table - TCL heap
  chprintf(chp, "%s%s %u %s", HTML_tr_th, TEXT_Heap, UMM_MALLOC_CFG_HEAP_SIZE/1024, TEXT_kB);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Used);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Free);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Total);
  chprintf(chp, "%s%s%s", HTML_e_th_th, TEXT_Metric, HTML_e_th_e_tr);
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Entries, HTML_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.usedEntries, HTML_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.freeEntries, HTML_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.totalEntries, HTML_e_td_td);
  chprintf(chp, "%s %u%%%s", TEXT_Fragmentation, umm_fragmentation_metric(), HTML_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", TEXT_Blocks, HTML_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.usedBlocks, HTML_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.freeBlocks, HTML_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.totalBlocks, HTML_e_td_td);
  chprintf(chp, "%s %u%%", TEXT_Used, umm_usage_metric());
  chprintf(chp, "%s%s %u %s", HTML_tr_th, TEXT_Storage, (UBS_BLOCK_SIZE * UBS_BLOCK_COUNT)/1024, TEXT_kB);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Used);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Free);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Total);
  chprintf(chp, "%s%s%s", HTML_e_th_th, TEXT_Metric, HTML_e_th_e_tr);
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Blocks, HTML_e_td_td);
  chprintf(chp, "%u%s", UBS_BLOCK_COUNT - uBSFreeBlocks, HTML_e_td_td);
  chprintf(chp, "%u%s", uBSFreeBlocks, HTML_e_td_td);
  chprintf(chp, "%u%s", UBS_BLOCK_COUNT, HTML_e_td_td);
  chprintf(chp, "%s %u%%", TEXT_Used, (uint8_t)((UBS_BLOCK_COUNT - uBSFreeBlocks) / (float)(UBS_BLOCK_COUNT / (float)100)));
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);

  chprintf(chp, "<i class='icon' style='font-size:20px;' title=\"");
  tcl_list_cmd(&tcl, &chp, "\r\n", 0b00111100);
  chprintf(chp, "\">&#xf121;</i>");
  chprintf(chp, "<i class='icon' style='font-size:20px;' title=\"");
  tcl_list_var(&tcl, &chp, "\r\n");
  chprintf(chp, "\">&#xf292;</i>");
  chprintf(chp, "%s%s", HTML_br, HTML_br);
  // Script area
  printTextArea(chp, 's', tclCmd, TCL_SCRIPT_LENGTH, 120, 20);
  chprintf(chp, "%s%s", HTML_br, HTML_br);
  // Select script
  chprintf(chp, "%s%s%s%s", HTML_table, HTML_tr_td, TEXT_Script, HTML_e_td_td);

  chprintf(chp, "%sP%s", HTML_select_submit, HTML_e_tag);
  logAddress = 1;
  for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
    chprintf(chp, "%s%u", HTML_option, logAddress);
    if (webScript == logAddress) { chprintf(chp, "%s", HTML_selected); }
    else                         { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%u. %s%s", logAddress, scriptp->name, HTML_e_option);
    logAddress++;
  }
  chprintf(chp, "%s%u", HTML_option, DUMMY_NO_VALUE);
  if (webScript == DUMMY_NO_VALUE) { chprintf(chp, "%s", HTML_selected); }
  else                             { chprintf(chp, "%s", HTML_e_tag); }
  chprintf(chp, "New script%s", HTML_e_option);
  chprintf(chp, "%s%s", HTML_e_select, HTML_e_td_e_tr_tr_td);

  chprintf(chp, "%s%s", TEXT_Name, HTML_e_td_td);
  printTextInput(chp, 'n', scriptName, NAME_LENGTH);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  // Buttons
  chprintf(chp, "%s%s%s%s", HTML_Run, HTML_Refresh, HTML_Save, HTML_Restart);
  // Output
  chprintf(chp, "%s<pre>Last output:\r\n%.*s</pre>", HTML_br, strlen(tclOutput), tclOutput);
  //for (uint16_t j = 0; j < strlen(tclOutput); j++) { DBG_HTTP("-%x", tclOutput[j]); }
  //DBG_HTTP("\r\n");
}

/*
 * @brief HTTP TCL POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_tcl(char **postDataP) {
  uint16_t number, valueLen = 0;
  int8_t resp;
  char name[3];
  bool repeat;
  char *valueP;
  scriptEvent_t *outMsg;
  // Example POST data:
  // first select: s=&P=4&n=\0
  // save existing: s=1234567890123456789012345678901234567890123456789010%134%15%16%178%19%10%40&P=4&n=2&e=Save\0
  //

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 's': // script
        strncpy(tclCmd, valueP, LWIP_MIN(valueLen, TCL_SCRIPT_LENGTH - 1));
        tclCmd[LWIP_MIN(valueLen, TCL_SCRIPT_LENGTH - 1)] = 0;
        break;
      case 'P': // select script
        number = strtol(valueP, NULL, 10);
        if (number != webScript) {
          DBG_HTTP("Select script %u\r\n", number);
          webScript = number;
          repeat = 0; // Force exit from loop to reload script
          // Clear script and name
          memset(&tclCmd[0], '\0', TCL_SCRIPT_LENGTH);
          memset(&scriptName[0], '\0', NAME_LENGTH);
          // For old script load values
          if (webScript != DUMMY_NO_VALUE) {
            number = 1;
            for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
              if (number == webScript) break;
              number++;
            }
            if (scriptp != NULL) {
              // Load script and name
              DBG_HTTP("Load script '%s'\r\n", scriptp->name);
              strncpy(&scriptName[0], scriptp->name, NAME_LENGTH);
              strncpy(&tclCmd[0], scriptp->cmd, TCL_SCRIPT_LENGTH);
            }
          }
        }
        break;
      case 'n': // name
        // For existing scripts do rename when name differs
        if ((strcmp(scriptName, valueP)) && (webScript != DUMMY_NO_VALUE)) {
          DBG_HTTP("Rename: '%s' -> '%.*s'\r\n", scriptName, valueLen, valueP);
          if (valueLen == 0) {
            chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, TEXT_error_name_empty);
            httpAlert.type = ALERT_ERROR;
            break;
          }
          number = 1;
          // Find pointer to script
          for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
            if (number == webScript) break;
            number++;
          }
          memset(scriptp->name, 0, NAME_LENGTH); // Make sure all is 0, as it gets stored in uBS
          strncpy(scriptp->name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
          // uBS rename
          resp = uBSRename(scriptName, scriptp->name);
          if (resp != UBS_RSLT_OK) {
            DBG_HTTP("Rename error: %d\r\n", resp);
            chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, TEXT_error_rename);
            httpAlert.type = ALERT_ERROR;
            repeat = 0; // exit
            break;
          }
        }
        // else just copy name for new script
        memset(scriptName, 0, NAME_LENGTH);
        strncpy(scriptName, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        break;
      case 'R': // Run
        outMsg = chPoolAlloc(&script_pool);
        if (outMsg != NULL) {
          outMsg->callback = NULL;
          outMsg->result = NULL;
          outMsg->flags = 1;
          outMsg->cmdP = &tclCmd[0];
          msg_t msg = chMBPostTimeout(&script_mb, (msg_t)outMsg, TIME_IMMEDIATE);
          if (msg != MSG_OK) {
            //chprintf(console, "MB full %d\r\n", temp);
          }
        } else {
          chprintf(console, "CB full %d \r\n", outMsg);
        }
      break;
      case 'e': // save
        if (webScript == DUMMY_NO_VALUE) {
          if (!strlen(scriptName)) {
            chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, "Not allowed to save empty name");
            httpAlert.type = ALERT_ERROR;
            break;
          }
          // For new script append linked list
          scriptp = umm_malloc(sizeof(struct scriptLL_t));
          if (scriptp == NULL) {
            chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, TEXT_error_free, TEXT_heap);
            httpAlert.type = ALERT_ERROR;
          } else {
            scriptp->name = umm_malloc(NAME_LENGTH);
            if (scriptp->name == NULL) {
              umm_free(scriptp);
              chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, TEXT_error_free, TEXT_heap);
              httpAlert.type = ALERT_ERROR;
            } else {
              strncpy(scriptp->name, &scriptName[0], NAME_LENGTH);
              number = strlen(tclCmd); // number as size of new script
              scriptp->cmd = umm_malloc(number + 1); // + NULL
              if (scriptp->cmd == NULL) {
                umm_free(scriptp->name);
                umm_free(scriptp);
                chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, "%s%s", TEXT_error_free, TEXT_heap);
                httpAlert.type = ALERT_ERROR;
              } else {
                strncpy(scriptp->cmd, &tclCmd[0], number);
                scriptp->cmd[number] = 0;
                scriptp->next = scriptLL;
                scriptLL = scriptp;
                // uBS
                if (uBSWrite(&scriptName[0], &tclCmd[0], number) != UBS_RSLT_OK) {
                  chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, TEXT_error_free, TEXT_storage);
                  httpAlert.type = ALERT_ERROR;
                }
                // new script is added to top of linked list, no need to do pointer check
                webScript = 1;
              }
            }
          }
        } else {
          // For old script replace values
          number = 1;
          // Find pointer to script
          for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
            if (number == webScript) break;
            number++;
          }
          if (scriptp != NULL) {
            number = strlen(tclCmd); // number as size of new script
            scriptp->cmd = umm_realloc(scriptp->cmd, number + 1);
            if (scriptp->cmd == NULL) {
              chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, "%s%s", TEXT_error_free, TEXT_heap);
              httpAlert.type = ALERT_ERROR;
            } else {
              strncpy(scriptp->cmd, &tclCmd[0], number);
              scriptp->cmd[number] = 0;
              if (uBSWrite(scriptName, &tclCmd[0], number) != UBS_RSLT_OK) {
                chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, TEXT_error_free, TEXT_storage);
                httpAlert.type = ALERT_ERROR;
              }
            }
          } else {
            chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, TEXT_error_script_not_found);
            httpAlert.type = ALERT_ERROR;
          }
        }
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_TCL_H_ */

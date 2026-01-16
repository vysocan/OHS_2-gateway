/*
 * httpd_handler_tcl.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_TCL_H_
#define HTTPD_HANDLER_TCL_H_


static void fs_open_custom_tcl(BaseSequentialStream *chp) {
  uint16_t logAddress;
  // Information table - TCL heap
  chprintf(chp, "%s%s %u %s", html_tr_th, text_Heap, UMM_MALLOC_CFG_HEAP_SIZE/1024, text_kB);
  chprintf(chp, "%s%s", html_e_th_th, text_Used);
  chprintf(chp, "%s%s", html_e_th_th, text_Free);
  chprintf(chp, "%s%s", html_e_th_th, text_Total);
  chprintf(chp, "%s%s%s", html_e_th_th, text_Metric, html_e_th_e_tr);
  chprintf(chp, "%s%s%s", html_tr_td, text_Entries, html_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.usedEntries, html_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.freeEntries, html_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.totalEntries, html_e_td_td);
  chprintf(chp, "%s %u%%%s", text_Fragmentation, umm_fragmentation_metric(), html_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", text_Blocks, html_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.usedBlocks, html_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.freeBlocks, html_e_td_td);
  chprintf(chp, "%u%s", ummHeapInfo.totalBlocks, html_e_td_td);
  chprintf(chp, "%s %u%%", text_Used, umm_usage_metric());
  chprintf(chp, "%s%s %u %s", html_tr_th, text_Storage, (UBS_BLOCK_SIZE * UBS_BLOCK_COUNT)/1024, text_kB);
  chprintf(chp, "%s%s", html_e_th_th, text_Used);
  chprintf(chp, "%s%s", html_e_th_th, text_Free);
  chprintf(chp, "%s%s", html_e_th_th, text_Total);
  chprintf(chp, "%s%s%s", html_e_th_th, text_Metric, html_e_th_e_tr);
  chprintf(chp, "%s%s%s", html_tr_td, text_Blocks, html_e_td_td);
  chprintf(chp, "%u%s", UBS_BLOCK_COUNT - uBSFreeBlocks, html_e_td_td);
  chprintf(chp, "%u%s", uBSFreeBlocks, html_e_td_td);
  chprintf(chp, "%u%s", UBS_BLOCK_COUNT, html_e_td_td);
  chprintf(chp, "%s %u%%", text_Used, (uint8_t)((UBS_BLOCK_COUNT - uBSFreeBlocks) / (float)(UBS_BLOCK_COUNT / (float)100)));
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

  chprintf(chp, "<i class='icon' style='font-size:20px;' title=\"");
  tcl_list_cmd(&tcl, &chp, "\r\n", 0b00111100);
  chprintf(chp, "\">&#xf121;</i>");
  chprintf(chp, "<i class='icon' style='font-size:20px;' title=\"");
  tcl_list_var(&tcl, &chp, "\r\n");
  chprintf(chp, "\">&#xf292;</i>");
  chprintf(chp, "%s%s", html_br, html_br);
  // Script area
  printTextArea(chp, 's', tclCmd, TCL_SCRIPT_LENGTH, 120, 20);
  chprintf(chp, "%s%s", html_br, html_br);
  // Select script
  chprintf(chp, "%s%s%s%s", html_table, html_tr_td, text_Script, html_e_td_td);

  chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
  logAddress = 1;
  for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
    chprintf(chp, "%s%u", html_option, logAddress);
    if (webScript == logAddress) { chprintf(chp, "%s", html_selected); }
    else                         { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u. %s%s", logAddress, scriptp->name, html_e_option);
    logAddress++;
  }
  chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
  if (webScript == DUMMY_NO_VALUE) { chprintf(chp, "%s", html_selected); }
  else                             { chprintf(chp, "%s", html_e_tag); }
  chprintf(chp, "New script%s", html_e_option);
  chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);

  chprintf(chp, "%s%s", text_Name, html_e_td_td);
  printTextInput(chp, 'n', scriptName, NAME_LENGTH);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s%s%s%s", html_Run, html_Refresh, html_Save, html_Restart);
  // Output
  chprintf(chp, "%s<pre>Last output:\r\n%.*s</pre>", html_br, strlen(tclOutput), tclOutput);
  //for (uint16_t j = 0; j < strlen(tclOutput); j++) { DBG_HTTP("-%x", tclOutput[j]); }
  //DBG_HTTP("\r\n");
}


#endif /* HTTPD_HANDLER_TCL_H_ */

/*
 * ohs_http_const.h
 *
 *  Created on: Apr 28, 2021
 *      Author: vysocan
 */

#ifndef OHS_HTTP_CONST_H_
#define OHS_HTTP_CONST_H_

// Icons
const char html_i_home[]            = "<i class='icon'>&#xe800;</i>";
const char html_i_contact[]         = "<i class='icon'>&#xe801;</i>";
const char html_i_key[]             = "<i class='icon'>&#xe802;</i>";
const char html_i_alert[]           = "<i class='icon'>&#xe803;</i>";
const char html_i_time[]            = "<i class='icon'>&#xe804;</i>";
const char html_i_OK[]              = "<i class='icon'>&#xe805;</i>";
const char html_i_disabled[]        = "<i class='icon'>&#xe806;</i>";
const char html_i_setting[]         = "<i class='icon'>&#xe807;</i>";
const char html_i_calendar[]        = "<i class='icon'>&#xe808;</i>";
const char html_i_globe[]           = "<i class='icon'>&#xe809;</i>";
const char html_i_lock[]            = "<i class='icon'>&#xe80a;</i>";
const char html_i_lock_closed[]     = "<i class='icon'>&#xe80b;</i>";
const char html_i_zone[]            = "<i class='icon'>&#xf096;</i>";
const char html_i_network[]         = "<i class='icon'>&#xf0e8;</i>";
const char html_i_alarm[]           = "<i class='icon'>&#xf0f3;</i>";
const char html_i_starting[]        = "<i class='icon'>&#xf110;</i>";
const char html_i_code[]            = "<i class='icon'>&#xf121;</i>";
const char html_i_question[]        = "<i class='icon'>&#xf191;</i>";
const char html_i_trigger[]         = "<i class='icon'>&#xf192;</i>";
const char html_i_script[]          = "<i class='icon'>&#xf1c9;</i>";
const char html_i_option[]          = "<i class='icon'>&#xf1de;</i>";
const char html_i_nodes[]           = "<i class='icon'>&#xf1e0;</i>";
const char html_i_group[]           = "<i class='icon'>&#xf24d;</i>";
const char html_i_hash[]            = "<i class='icon'>&#xf292;</i>";

// Tags
const char html_tr_td[]             = "<tr><td>";
const char html_e_td_td[]           = "</td><td>";
const char html_e_td_e_tr[]         = "</td></tr>";
const char html_e_td_e_tr_tr_td[]   = "</td></tr><tr><td>";
const char html_tr_th[]             = "<tr><th>";
const char html_e_th_th[]           = "</th><th>";
const char html_e_th_e_tr[]         = "</th></tr>";
const char html_select_submit[]     = "<select onchange='this.form.submit()' name='";
const char html_e_tag[]             = "'>";
const char html_e_select[]          = "</select>";
const char html_option[]            = "<option value='";
const char html_e_option[]          = "</option>";
const char html_selected[]          = "' selected>";
const char html_m_tag[]             = "' value='";
const char html_id_tag[]            = "' id='";
const char html_t_tag_1[]           = "<input type='text' maxlength='";
const char html_i_tag_1[]           = "<input type='time";
const char html_i_tag_2[]           = "' min='00:00' max='23:59' required>";
const char html_n_tag_1[]           = "<input type='number' style='width:";
const char html_n_tag_2[]           = "em' min='";
const char html_n_tag_3[]           = "' max='";
const char html_p_tag_1[]           = "<input type='password' maxlength='";
const char html_s_tag_2[]           = "' size='";
const char html_s_tag_3[]           = "' name='";
const char html_s_tag_4[]           = "' minlength='";
const char html_radio_s[]           = "<div class='rm'>";
const char html_radio_sl[]          = "<div class='rml'>";
const char html_radio_sb[]          = "<div class='rmb'>";
const char html_div_e[]             = "</div>";
const char html_div_id_1[]          = "<div id='hd_";
const char html_div_id_2[]          = "' style='display:block;'>";
const char html_select[]            = "<select name='";
const char html_Apply[]             = "<input type='submit' name='A' value='Apply'/>";
const char html_ApplyValPass[]      = "<input type='submit' name='A' value='Apply' onclick='return pv()'/>";
const char html_Save[]              = "<input type='submit' name='e' value='Save'/>";
const char html_Disarm[]            = "<input type='submit' name='D' value='Disarm'/>";
const char html_LoadDefault[]       = "<input type='submit' name='D' value='Load defaults'/>";
const char html_Reregister[]        = "<input type='submit' name='R' value='Call registration'/>";
const char html_Now[]               = "<input type='submit' name='N' value='Now'/>";
const char html_FR[]                = "<input type='submit' name='R' value='<<'/>";
const char html_R[]                 = "<input type='submit' name='r' value='<'/>";
const char html_FF[]                = "<input type='submit' name='F' value='>>'/>";
const char html_F[]                 = "<input type='submit' name='f' value='>'/>";
const char html_Run[]               = "<input type='submit' name='R' value='Run'/>";
const char html_Refresh[]           = "<input type='submit' name='F' value='Refresh'/>";
const char html_Restart[]           = "<input type='submit' name='S' value='Restart'/>";
const char html_textarea_1[]        = "<textarea name='";
const char html_textarea_2[]        = "' id='";
const char html_textarea_3[]        = "' rows='";
const char html_textarea_4[]        = "' cols='";
const char html_textarea_5[]        = "' maxlength='";
const char html_textarea_e[]        = "</textarea>";
const char html_e_table[]           = "</table>";
const char html_table[]             = "<table>";
const char html_form_1[]            = "<form action='";
const char html_form_2[]            = "' method='post'>";
const char html_br[]                = "<br>";

// Radio buttons
const char html_cbPart1a[]          = "<div class='rc'><input type='radio' name='";
const char html_cbPart1b[]          = "' id='";
const char html_cbPart2[]           = "' value='";
const char html_cbPart3[]           = "'";
const char html_cbChecked[]         = "' checked";
const char html_cbPart4a[]          = "><label for='";
const char html_cbPart4b[]          = "'>";
const char html_cbPart5[]           = "</label></div>";
const char html_cbJSen[]            = " onclick=\"en";
const char html_cbJSdis[]           = " onclick=\"dis";
const char html_cbJSend[]           = "()\"";

// JavaScript related
const char html_script[]            = "<script>";
const char html_e_script[]          = "</script>";
const char html_script_src[]        = "<script type='text/javascript' src='/js/";
const char JSen1[]                  = "en1();";
const char JSen2[]                  = "en2();";
const char JSdis1[]                 = "dis1();";
const char JSdis2[]                 = "dis2();";
const char JSContact[]              = "var e1=document.querySelectorAll(\"#g\");"
                                      "var d1=document.querySelectorAll(\"#xx\");";
const char JSCredential[]           = "var tc=document.querySelectorAll(\"#p\");";
const char JSZone[]                 = "var e1=document.querySelectorAll(\"#xx\");"
                                      "var d1=document.querySelectorAll(\"#a1,#a0\");";
const char JSTimer[]                = "var e1=document.querySelectorAll(\"#s,#S\");"
                                      "var d1=document.querySelectorAll(\"#D0,#D1,#E0,#E1,#F0,#F1,#G0,#G1,#H0,#H1,#I0,#I1,#J0,#J1\");";
const char JSTrigger[]              = "var e1=document.querySelectorAll(\"#xx\");"
                                      "var e2=document.querySelectorAll(\"#xx\");"
                                      "var d1=document.querySelectorAll(\"#t,#T,#S0,#S1,#S2,#a,#o,#f\");"
                                      "var d2=document.querySelectorAll(\"#t,#T\");";
const char JSTriggerSel_1[]         = "function sd(select){if(select.value<";
const char JSTriggerSel_2[]         = "){"
                                      "document.getElementById('hd_1').style.display='block';"
                                      "document.getElementById('hd_2').style.display='none';"
                                      "document.getElementById('hd_3').style.display='none';"
                                      "document.getElementById('h').disabled=true;"
                                      "}else if((select.value>=";
const char JSTriggerSel_3[]         = ")&&(select.value<";
const char JSTriggerSel_4[]         = ")){"
                                      "document.getElementById('hd_1').style.display='none';"
                                      "document.getElementById('hd_2').style.display='block';"
                                      "document.getElementById('hd_3').style.display='none';"
                                      "document.getElementById('h').disabled=true;}else{"
                                      "document.getElementById('hd_1').style.display='none';"
                                      "document.getElementById('hd_2').style.display='none';"
                                      "document.getElementById('hd_3').style.display='block';"
                                      "document.getElementById('h').disabled=false;"
                                      "}}";

#endif /* OHS_HTTP_CONST_H_ */

/*
 * ohs_http_const.h
 *
 *  Created on: Apr 28, 2021
 *      Author: vysocan
 */

#ifndef OHS_HTTP_CONST_H_
#define OHS_HTTP_CONST_H_

// Icons
#define HTML_i_home                   "<i class='icon'>&#xe800;</i>"
#define HTML_i_contact                "<i class='icon'>&#xe801;</i>"
#define HTML_i_key                    "<i class='icon'>&#xe802;</i>"
#define HTML_i_alert                  "<i class='icon'>&#xe803;</i>"
#define HTML_i_time                   "<i class='icon'>&#xe804;</i>"
#define HTML_i_OK                     "<i class='icon'>&#xe805;</i>"
#define HTML_i_disabled               "<i class='icon'>&#xe806;</i>"
#define HTML_i_setting                "<i class='icon'>&#xe807;</i>"
#define HTML_i_calendar               "<i class='icon'>&#xe808;</i>"
#define HTML_i_globe                  "<i class='icon'>&#xe809;</i>"
#define HTML_i_lock                   "<i class='icon'>&#xe80a;</i>"
#define HTML_i_lock_closed            "<i class='icon'>&#xe80b;</i>"
#define HTML_i_zone                   "<i class='icon'>&#xf096;</i>"
#define HTML_i_network                "<i class='icon'>&#xf0e8;</i>"
#define HTML_i_alarm                  "<i class='icon'>&#xf0f3;</i>"
#define HTML_i_starting               "<i class='icon'>&#xf110;</i>"
#define HTML_i_code                   "<i class='icon'>&#xf121;</i>"
#define HTML_i_question               "<i class='icon'>&#xf191;</i>"
#define HTML_i_trigger                "<i class='icon'>&#xf192;</i>"
#define HTML_i_script                 "<i class='icon'>&#xf1c9;</i>"
#define HTML_i_option                 "<i class='icon'>&#xf1de;</i>"
#define HTML_i_nodes                  "<i class='icon'>&#xf1e0;</i>"
#define HTML_i_group                  "<i class='icon'>&#xf24d;</i>"
#define HTML_i_hash                   "<i class='icon'>&#xf292;</i>"

// Tags
#define HTML_tr_td                    "<tr><td>"
#define HTML_e_td_td                  "</td><td>"
#define HTML_e_td_e_tr                "</td></tr>"
#define HTML_e_td_e_tr_tr_td          "</td></tr><tr><td>"
#define HTML_tr_th                    "<tr><th>"
#define HTML_e_th_th                  "</th><th>"
#define HTML_e_th_e_tr                "</th></tr>"
#define HTML_select_submit            "<select onchange='this.form.submit()' name='"
#define HTML_e_tag                    "'>"
#define HTML_e_select                 "</select>"
#define HTML_option                   "<option value='"
#define HTML_e_option                 "</option>"
#define HTML_selected                 "' selected>"
#define HTML_m_tag                    "' value='"
#define HTML_id_tag                   "' id='"
#define HTML_t_tag_1                  "<input type='text' maxlength='"
#define HTML_i_tag_1                  "<input type='time"
#define HTML_i_tag_2                  "' min='00:00' max='23:59' required>"
#define HTML_n_tag_1                  "<input type='number' style='width:"
#define HTML_n_tag_2                  "em' min='"
#define HTML_n_tag_3                  "' max='"
#define HTML_p_tag_1                  "<input type='password' maxlength='"
#define HTML_s_tag_2                  "' size='"
#define HTML_s_tag_3                  "' name='"
#define HTML_s_tag_4                  "' minlength='"
#define HTML_radio_s                  "<div class='rm'>"
#define HTML_radio_sl                 "<div class='rml'>"
#define HTML_radio_sb                 "<div class='rmb'>"
#define HTML_div_e                    "</div>"
#define HTML_div_id_1                 "<div id='hd_"
#define HTML_div_id_2                 "' style='display:block;'>"
#define HTML_select                   "<select name='"
#define HTML_Apply                    "<input type='submit' name='A' value='Apply'/>"
#define HTML_Enroll                   "<input type='submit' name='E' value='Enroll FP'/>"
#define HTML_Delete                   "<input type='submit' name='D' value='Delete FP'/>"
#define HTML_ApplyValPass             "<input type='submit' name='A' value='Apply' onclick='return pv()'/>"
#define HTML_Save                     "<input type='submit' name='e' value='Save'/>"
#define HTML_Disarm                   "<input type='submit' name='D' value='Disarm'/>"
#define HTML_LoadDefault              "<input type='submit' name='D' value='Load defaults'/>"
#define HTML_Reregister               "<input type='submit' name='R' value='Call registration'/>"
#define HTML_SendTest                 "<input type='submit' name='M' value='Send Test'/>"
#define HTML_Now                      "<input type='submit' name='N' value='Now'/>"
#define HTML_FR                       "<input type='submit' name='R' value='<<'/>"
#define HTML_R                        "<input type='submit' name='r' value='<'/>"
#define HTML_FF                       "<input type='submit' name='F' value='>>'/>"
#define HTML_F                        "<input type='submit' name='f' value='>'/>"
#define HTML_Run                      "<input type='submit' name='R' value='Run'/>"
#define HTML_Refresh                  "<input type='submit' name='F' value='Refresh'/>"
#define HTML_Restart                  "<input type='submit' name='S' value='Restart'/>"
#define HTML_Submit                   "<input type='submit' name='S' value='Submit'/>"
#define HTML_textarea_1               "<textarea name='"
#define HTML_textarea_2               "' id='"
#define HTML_textarea_3               "' rows='"
#define HTML_textarea_4               "' cols='"
#define HTML_textarea_5               "' maxlength='"
#define HTML_textarea_e               "</textarea>"
#define HTML_e_table                  "</table>"
#define HTML_table                    "<table>"
#define HTML_form_1                   "<form action='"
#define HTML_form_2                   "' method='post'>"
#define HTML_br                       "<br>"
#define HTML_Download                 "<button type='button' onclick=\"window.open('/config.bin')\">Download config</button>"
#define HTML_Upload                   "<label class='cf'><input type='file' id='u' onchange='uC(this)'>Upload config</label>"
#define HTML_js_upload                "<script>function uC(e){var f=e.files[0];if(!f)return;var r=new FileReader();r.onload=function(e){var x=new XMLHttpRequest();x.onload=function(){if(x.status==200){document.open();document.write(x.responseText);document.close();}else{alert('Error '+x.status);}};x.open('POST','/index.html',true);x.send(e.target.result);};r.readAsArrayBuffer(f);}</script>"

// Radio buttons
#define HTML_cbPart1a                 "<div class='rc'><input type='radio' name='"
#define HTML_cbPart1b                 "' id='"
#define HTML_cbPart2                  "' value='"
#define HTML_cbPart3                  "'"
#define HTML_cbChecked                "' checked"
#define HTML_cbPart4                  "><label for='"
#define HTML_cbPart5                  "</label></div>"
#define HTML_cbJSen                   " onclick=\"en"
#define HTML_cbJSdis                  " onclick=\"dis"
#define HTML_cbJSend                  "()\""

// JavaScript related
#define HTML_script                   "<script>"
#define HTML_e_script                 "</script>"
#define HTML_script_src               "<script type='text/javascript' src='/js/"
#define JS_en1                        "en1();"
#define JS_en2                        "en2();"
#define JS_dis1                       "dis1();"
#define JS_dis2                       "dis2();"
#define JS_Contact                    "var e1=document.querySelectorAll(\"#g\");" \
                                      "var d1=document.querySelectorAll(\"#xx\");"
#define JS_Credential                 "var tc=document.querySelectorAll(\"#p\");"
#define JS_Zone                       "var e1=document.querySelectorAll(\"#xx\");" \
                                      "var d1=document.querySelectorAll(\"#a1,#a0\");"
#define JS_Timer                      "var e1=document.querySelectorAll(\"#s,#S\");" \
                                      "var d1=document.querySelectorAll(\"#D0,#D1,#E0,#E1,#F0,#F1,#G0,#G1,#H0,#H1,#I0,#I1,#J0,#J1\");"
#define JS_Trigger                    "var e1=document.querySelectorAll(\"#xx\");" \
                                      "var e2=document.querySelectorAll(\"#xx\");" \
                                      "var d1=document.querySelectorAll(\"#t,#T,#S0,#S1,#S2,#a,#o,#f\");" \
                                      "var d2=document.querySelectorAll(\"#t,#T\");"
#define JS_TriggerSel_1               "function sd(select){if(select.value<"
#define JS_TriggerSel_2               "){" \
                                      "document.getElementById('hd_1').style.display='block';" \
                                      "document.getElementById('hd_2').style.display='none';" \
                                      "document.getElementById('hd_3').style.display='none';" \
                                      "document.getElementById('h').disabled=true;" \
                                      "}else if((select.value>="
#define JS_TriggerSel_3               ")&&(select.value<"
#define JS_TriggerSel_4               ")){" \
                                      "document.getElementById('hd_1').style.display='none';" \
                                      "document.getElementById('hd_2').style.display='block';" \
                                      "document.getElementById('hd_3').style.display='none';" \
                                      "document.getElementById('h').disabled=true;}else{" \
                                      "document.getElementById('hd_1').style.display='none';" \
                                      "document.getElementById('hd_2').style.display='none';" \
                                      "document.getElementById('hd_3').style.display='block';" \
                                      "document.getElementById('h').disabled=false;" \
                                      "}}"

// Range Slider Tags
#define HTML_range_tag_1              "<div class='slidc'><input type='range' autocomplete='off' class='slid' min='"
#define HTML_range_tag_2              "' max='"
#define HTML_range_oninput            "' oninput='document.getElementById(\"val_"
#define HTML_range_val_update         "\").innerHTML=this.value'"
#define HTML_range_output_1           "<span class='slidv' id='val_"

// Tooltip for table cells
#define HTML_tr_td_tip                "<tr><td><span class='tip' data-t='"
#define HTML_td_tip                   "<td><span class='tip' data-t='"
#define HTML_e_span                   "</span>"

#endif /* OHS_HTTP_CONST_H_ */

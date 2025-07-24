/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = modifiercompiler.wordwrap.php

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
<?php
/**
 * Smarty plugin
 *
 * @package Smarty
 * @subpackage PluginsModifierCompiler
 */

/**
 * Smarty wordwrap modifier plugin
 * 
 * Type:     modifier<br>
 * Name:     wordwrap<br>
 * Purpose:  wrap a string of text at a given length
 * 
 * @link http://smarty.php.net/manual/en/language.modifier.wordwrap.php wordwrap (Smarty online manual)
 * @author Uwe Tews 
 * @param array $params parameters
 * @return string with compiled code
 */
function smarty_modifiercompiler_wordwrap($params, $compiler)
{
    if (!isset($params[1])) {
        $params[1] = 80;
    } 
    if (!isset($params[2])) {
        $params[2] = '"\n"';
    } 
    if (!isset($params[3])) {
        $params[3] = 'false';
    } 
    $function = 'wordwrap';
    if (Smarty::$_MBSTRING) {
    if ($compiler->template->caching && ($compiler->tag_nocache | $compiler->nocache)) {
            $compiler->template->required_plugins['nocache']['wordwrap']['modifier']['file'] = SMARTY_PLUGINS_DIR .'shared.mb_wordwrap.php';
            $compiler->template->required_plugins['nocache']['wordwrap']['modifier']['function'] = 'smarty_mb_wordwrap';
        } else {
            $compiler->template->required_plugins['compiled']['wordwrap']['modifier']['file'] = SMARTY_PLUGINS_DIR .'shared.mb_wordwrap.php';
            $compiler->template->required_plugins['compiled']['wordwrap']['modifier']['function'] = 'smarty_mb_wordwrap';
        }
        $function = 'smarty_mb_wordwrap';
    }
    return $function . '(' . $params[0] . ',' . $params[1] . ',' . $params[2] . ',' . $params[3] . ')';
} 

?>
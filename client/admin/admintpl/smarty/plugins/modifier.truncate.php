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

   Source File Name = modifier.truncate.php

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
 * @subpackage PluginsModifier
 */
 
/**
 * Smarty truncate modifier plugin
 * 
 * Type:     modifier<br>
 * Name:     truncate<br>
 * Purpose:  Truncate a string to a certain length if necessary,
 *               optionally splitting in the middle of a word, and
 *               appending the $etc string or inserting $etc into the middle.
 * 
 * @link http://smarty.php.net/manual/en/language.modifier.truncate.php truncate (Smarty online manual)
 * @author Monte Ohrt <monte at ohrt dot com> 
 * @param string  $string      input string
 * @param integer $length      length of truncated text
 * @param string  $etc         end string
 * @param boolean $break_words truncate at word boundary
 * @param boolean $middle      truncate in the middle of text
 * @return string truncated string
 */
function smarty_modifier_truncate($string, $length = 80, $etc = '...', $break_words = false, $middle = false) {
    if ($length == 0)
        return '';

    if (Smarty::$_MBSTRING) {
        if (mb_strlen($string, Smarty::$_CHARSET) > $length) {
            $length -= min($length, mb_strlen($etc, Smarty::$_CHARSET));
            if (!$break_words && !$middle) {
                $string = preg_replace('/\s+?(\S+)?$/' . Smarty::$_UTF8_MODIFIER, '', mb_substr($string, 0, $length + 1, Smarty::$_CHARSET));
            } 
            if (!$middle) {
                return mb_substr($string, 0, $length, Smarty::$_CHARSET) . $etc;
            }
            return mb_substr($string, 0, $length / 2, Smarty::$_CHARSET) . $etc . mb_substr($string, - $length / 2, $length, Smarty::$_CHARSET);
        }
        return $string;
    }
    
    // no MBString fallback
    if (isset($string[$length])) {
        $length -= min($length, strlen($etc));
        if (!$break_words && !$middle) {
            $string = preg_replace('/\s+?(\S+)?$/', '', substr($string, 0, $length + 1));
        } 
        if (!$middle) {
            return substr($string, 0, $length) . $etc;
        }
        return substr($string, 0, $length / 2) . $etc . substr($string, - $length / 2);
    }
    return $string;
} 

?>
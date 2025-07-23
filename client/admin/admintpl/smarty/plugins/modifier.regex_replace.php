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

   Source File Name = modifier.regex_replace.php

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
 * Smarty regex_replace modifier plugin
 *
 * Type:     modifier<br>
 * Name:     regex_replace<br>
 * Purpose:  regular expression search/replace
 *
 * @link http://smarty.php.net/manual/en/language.modifier.regex.replace.php
 *          regex_replace (Smarty online manual)
 * @author Monte Ohrt <monte at ohrt dot com>
 * @param string       $string   input string
 * @param string|array $search   regular expression(s) to search for
 * @param string|array $replace  string(s) that should be replaced
 * @return string
 */
function smarty_modifier_regex_replace($string, $search, $replace)
{
    if(is_array($search)) {
        foreach($search as $idx => $s) {
            $search[$idx] = _smarty_regex_replace_check($s);
        }
    } else {
        $search = _smarty_regex_replace_check($search);
    }
    return preg_replace($search, $replace, $string);
}

/**
 * @param  string $search string(s) that should be replaced
 * @return string
 * @ignore
 */
function _smarty_regex_replace_check($search)
{
    // null-byte injection detection
    // anything behind the first null-byte is ignored
    if (($pos = strpos($search,"\0")) !== false) {
        $search = substr($search,0,$pos);
    }
    // remove eval-modifier from $search
    if (preg_match('!([a-zA-Z\s]+)$!s', $search, $match) && (strpos($match[1], 'e') !== false)) {
        $search = substr($search, 0, -strlen($match[1])) . preg_replace('![e\s]+!', '', $match[1]);
    }
    return $search;
}

?>
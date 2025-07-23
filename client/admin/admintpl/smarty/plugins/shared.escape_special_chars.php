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

   Source File Name = shared.escape_special_chars.php

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
 * Smarty shared plugin
 *
 * @package Smarty
 * @subpackage PluginsShared
 */

if (version_compare(PHP_VERSION, '5.2.3', '>=')) {
    /**
     * escape_special_chars common function
     *
     * Function: smarty_function_escape_special_chars<br>
     * Purpose:  used by other smarty functions to escape
     *           special chars except for already escaped ones
     *
     * @author   Monte Ohrt <monte at ohrt dot com>
     * @param string $string text that should by escaped
     * @return string
     */
    function smarty_function_escape_special_chars($string)
    {
        if (!is_array($string)) {
            $string = htmlspecialchars($string, ENT_COMPAT, Smarty::$_CHARSET, false);
        }
        return $string;
    }  
} else {         
    /**
     * escape_special_chars common function
     *
     * Function: smarty_function_escape_special_chars<br>
     * Purpose:  used by other smarty functions to escape
     *           special chars except for already escaped ones
     *
     * @author   Monte Ohrt <monte at ohrt dot com>
     * @param string $string text that should by escaped
     * @return string
     */
    function smarty_function_escape_special_chars($string)
    {
        if (!is_array($string)) {
            $string = preg_replace('!&(#?\w+);!', '%%%SMARTY_START%%%\\1%%%SMARTY_END%%%', $string);
            $string = htmlspecialchars($string);
            $string = str_replace(array('%%%SMARTY_START%%%', '%%%SMARTY_END%%%'), array('&', ';'), $string); 
        }
        return $string;
    }                                                                                                             
} 

?>
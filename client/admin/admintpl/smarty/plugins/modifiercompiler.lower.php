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

   Source File Name = modifiercompiler.lower.php

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
 * @package Smarty
 * @subpackage PluginsModifierCompiler
 */

/**
 * Smarty lower modifier plugin
 *
 * Type:     modifier<br>
 * Name:     lower<br>
 * Purpose:  convert string to lowercase
 *
 * @link http://www.smarty.net/manual/en/language.modifier.lower.php lower (Smarty online manual)
 * @author Monte Ohrt <monte at ohrt dot com>
 * @author Uwe Tews
 * @param array $params parameters
 * @return string with compiled code
 */

function smarty_modifiercompiler_lower($params, $compiler)
{
    if (Smarty::$_MBSTRING) {
        return 'mb_strtolower(' . $params[0] . ', \'' . addslashes(Smarty::$_CHARSET) . '\')' ;
    }
    // no MBString fallback
    return 'strtolower(' . $params[0] . ')';
}

?>
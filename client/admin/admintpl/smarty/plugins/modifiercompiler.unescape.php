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

   Source File Name = modifiercompiler.unescape.php

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
 * Smarty unescape modifier plugin
 *
 * Type:     modifier<br>
 * Name:     unescape<br>
 * Purpose:  unescape html entities
 *
 * @author Rodney Rehm
 * @param array $params parameters
 * @return string with compiled code
 */
function smarty_modifiercompiler_unescape($params, $compiler)
{
    if (!isset($params[1])) {
        $params[1] = 'html';
    }
    if (!isset($params[2])) {
        $params[2] = '\'' . addslashes(Smarty::$_CHARSET) . '\'';
    } else {
        $params[2] = "'" . $params[2] . "'";
    }

    switch (trim($params[1], '"\'')) {
        case 'entity':
        case 'htmlall':
            if (Smarty::$_MBSTRING) {
                return 'mb_convert_encoding(' . $params[0] . ', ' . $params[2] . ', \'HTML-ENTITIES\')';
            }

            return 'html_entity_decode(' . $params[0] . ', ENT_NOQUOTES, ' . $params[2] . ')';

        case 'html':
            return 'htmlspecialchars_decode(' . $params[0] . ', ENT_QUOTES)';

        case 'url':
            return 'rawurldecode(' . $params[0] . ')';

        default:
            return $params[0];
    }
}

?>
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

   Source File Name = function.math.php

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
 * This plugin is only for Smarty2 BC
 * @package Smarty
 * @subpackage PluginsFunction
 */

/**
 * Smarty {math} function plugin
 *
 * Type:     function<br>
 * Name:     math<br>
 * Purpose:  handle math computations in template
 *
 * @link http://www.smarty.net/manual/en/language.function.math.php {math}
 *          (Smarty online manual)
 * @author   Monte Ohrt <monte at ohrt dot com>
 * @param array                    $params   parameters
 * @param Smarty_Internal_Template $template template object
 * @return string|null
 */
function smarty_function_math($params, $template)
{
    static $_allowed_funcs = array(
        'int' => true, 'abs' => true, 'ceil' => true, 'cos' => true, 'exp' => true, 'floor' => true,
        'log' => true, 'log10' => true, 'max' => true, 'min' => true, 'pi' => true, 'pow' => true,
        'rand' => true, 'round' => true, 'sin' => true, 'sqrt' => true, 'srand' => true ,'tan' => true
    );
    // be sure equation parameter is present
    if (empty($params['equation'])) {
        trigger_error("math: missing equation parameter",E_USER_WARNING);
        return;
    }

    $equation = $params['equation'];

    // make sure parenthesis are balanced
    if (substr_count($equation,"(") != substr_count($equation,")")) {
        trigger_error("math: unbalanced parenthesis",E_USER_WARNING);
        return;
    }

    // match all vars in equation, make sure all are passed
    preg_match_all("!(?:0x[a-fA-F0-9]+)|([a-zA-Z][a-zA-Z0-9_]*)!",$equation, $match);

    foreach($match[1] as $curr_var) {
        if ($curr_var && !isset($params[$curr_var]) && !isset($_allowed_funcs[$curr_var])) {
            trigger_error("math: function call $curr_var not allowed",E_USER_WARNING);
            return;
        }
    }

    foreach($params as $key => $val) {
        if ($key != "equation" && $key != "format" && $key != "assign") {
            // make sure value is not empty
            if (strlen($val)==0) {
                trigger_error("math: parameter $key is empty",E_USER_WARNING);
                return;
            }
            if (!is_numeric($val)) {
                trigger_error("math: parameter $key: is not numeric",E_USER_WARNING);
                return;
            }
            $equation = preg_replace("/\b$key\b/", " \$params['$key'] ", $equation);
        }
    }
    $smarty_math_result = null;
    eval("\$smarty_math_result = ".$equation.";");

    if (empty($params['format'])) {
        if (empty($params['assign'])) {
            return $smarty_math_result;
        } else {
            $template->assign($params['assign'],$smarty_math_result);
        }
    } else {
        if (empty($params['assign'])){
            printf($params['format'],$smarty_math_result);
        } else {
            $template->assign($params['assign'],sprintf($params['format'],$smarty_math_result));
        }
    }
}

?>
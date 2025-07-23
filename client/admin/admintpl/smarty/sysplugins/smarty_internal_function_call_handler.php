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

   Source File Name = smarty_internal_function_call_handler.php

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
 * Smarty Internal Plugin Function Call Handler
 *
 * @package Smarty
 * @subpackage PluginsInternal
 * @author Uwe Tews
 */

/**
 * This class does call function defined with the {function} tag
 *
 * @package Smarty
 * @subpackage PluginsInternal
 */
class Smarty_Internal_Function_Call_Handler {

    /**
     * This function handles calls to template functions defined by {function}
     * It does create a PHP function at the first call
     *
     * @param string                   $_name       template function name
     * @param Smarty_Internal_Template $_template   template object
     * @param array                    $_params     Smarty variables passed as call parameter
     * @param string                   $_hash       nocache hash value
     * @param bool                     $_nocache    nocache flag
     */
    public static function call($_name, Smarty_Internal_Template $_template, $_params, $_hash, $_nocache)
    {
        if ($_nocache) {
            $_function = "smarty_template_function_{$_name}_nocache";
        } else {
            $_function = "smarty_template_function_{$_hash}_{$_name}";
        }
        if (!is_callable($_function)) {
            $_code = "function {$_function}(\$_smarty_tpl,\$params) {
    \$saved_tpl_vars = \$_smarty_tpl->tpl_vars;
    foreach (\$_smarty_tpl->smarty->template_functions['{$_name}']['parameter'] as \$key => \$value) {\$_smarty_tpl->tpl_vars[\$key] = new Smarty_variable(\$value);};
    foreach (\$params as \$key => \$value) {\$_smarty_tpl->tpl_vars[\$key] = new Smarty_variable(\$value);}?>";
            if ($_nocache) {
                $_code .= preg_replace(array("!<\?php echo \\'/\*%%SmartyNocache:{$_template->smarty->template_functions[$_name]['nocache_hash']}%%\*/|/\*/%%SmartyNocache:{$_template->smarty->template_functions[$_name]['nocache_hash']}%%\*/\\';\?>!",
                        "!\\\'!"), array('', "'"), $_template->smarty->template_functions[$_name]['compiled']);
                $_template->smarty->template_functions[$_name]['called_nocache'] = true;
            } else {
                $_code .= preg_replace("/{$_template->smarty->template_functions[$_name]['nocache_hash']}/", $_template->properties['nocache_hash'], $_template->smarty->template_functions[$_name]['compiled']);
            }
            $_code .= "<?php \$_smarty_tpl->tpl_vars = \$saved_tpl_vars;}";
            eval($_code);
        }
        $_function($_template, $_params);
    }

}

?>

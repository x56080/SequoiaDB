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

   Source File Name = smarty_internal_compile_include_php.php

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
 * Smarty Internal Plugin Compile Include PHP
 *
 * Compiles the {include_php} tag
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Insert Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Include_Php extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $required_attributes = array('file');
    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $shorttag_order = array('file');
    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('once', 'assign');

    /**
     * Compiles code for the {include_php} tag
     *
     * @param array  $args     array with attributes from parser
     * @param object $compiler compiler object
     * @return string compiled code
     */
    public function compile($args, $compiler)
    {
        if (!($compiler->smarty instanceof SmartyBC)) {
            throw new SmartyException("{include_php} is deprecated, use SmartyBC class to enable");
        }
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);

        $_output = '<?php ';

        $_smarty_tpl = $compiler->template;
        $_filepath = false;
        eval('$_file = ' . $_attr['file'] . ';');
        if (!isset($compiler->smarty->security_policy) && file_exists($_file)) {
            $_filepath = $_file;
        } else {
            if (isset($compiler->smarty->security_policy)) {
                $_dir = $compiler->smarty->security_policy->trusted_dir;
            } else {
                $_dir = $compiler->smarty->trusted_dir;
            }
            if (!empty($_dir)) {
                foreach((array)$_dir as $_script_dir) {
                    $_script_dir = rtrim($_script_dir, '/\\') . DS;
                    if (file_exists($_script_dir . $_file)) {
                        $_filepath = $_script_dir .  $_file;
                        break;
                    }
                }
            }
        }
        if ($_filepath == false) {
            $compiler->trigger_template_error("{include_php} file '{$_file}' is not readable", $compiler->lex->taglineno);
        }

        if (isset($compiler->smarty->security_policy)) {
            $compiler->smarty->security_policy->isTrustedPHPDir($_filepath);
        }

        if (isset($_attr['assign'])) {
            // output will be stored in a smarty variable instead of being displayed
            $_assign = $_attr['assign'];
        }
        $_once = '_once';
        if (isset($_attr['once'])) {
            if ($_attr['once'] == 'false') {
                $_once = '';
            }
        }

        if (isset($_assign)) {
            return "<?php ob_start(); include{$_once} ('{$_filepath}'); \$_smarty_tpl->assign({$_assign},ob_get_contents()); ob_end_clean();?>";
        } else {
            return "<?php include{$_once} ('{$_filepath}');?>\n";
        }
    }

}

?>
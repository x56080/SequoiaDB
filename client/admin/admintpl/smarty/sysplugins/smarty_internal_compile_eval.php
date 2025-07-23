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

   Source File Name = smarty_internal_compile_eval.php

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
 * Smarty Internal Plugin Compile Eval
 *
 * Compiles the {eval} tag.
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Eval Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Eval extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $required_attributes = array('var');
    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('assign');
    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $shorttag_order = array('var','assign');

    /**
     * Compiles code for the {eval} tag
     *
     * @param array  $args     array with attributes from parser
     * @param object $compiler compiler object
     * @return string compiled code
     */
    public function compile($args, $compiler)
    {
        $this->required_attributes = array('var');
        $this->optional_attributes = array('assign');
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);
        if (isset($_attr['assign'])) {
              // output will be stored in a smarty variable instead of beind displayed
            $_assign = $_attr['assign'];
        }

        // create template object
        $_output = "\$_template = new {$compiler->smarty->template_class}('eval:'.".$_attr['var'].", \$_smarty_tpl->smarty, \$_smarty_tpl);";
        //was there an assign attribute?
        if (isset($_assign)) {
            $_output .= "\$_smarty_tpl->assign($_assign,\$_template->fetch());";
        } else {
            $_output .= "echo \$_template->fetch();";
        }
        return "<?php $_output ?>";
    }

}

?>
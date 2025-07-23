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

   Source File Name = smarty_internal_compile_append.php

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
 * Smarty Internal Plugin Compile Append
 *
 * Compiles the {append} tag
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Append Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Append extends Smarty_Internal_Compile_Assign {

    /**
     * Compiles code for the {append} tag
     *
     * @param array $args array with attributes from parser
     * @param object $compiler compiler object
     * @param array $parameter array with compilation parameter
     * @return string compiled code
     */
    public function compile($args, $compiler, $parameter)
    {
        // the following must be assigned at runtime because it will be overwritten in parent class
        $this->required_attributes = array('var', 'value');
        $this->shorttag_order = array('var', 'value');
        $this->optional_attributes = array('scope', 'index');
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);
        // map to compile assign attributes
        if (isset($_attr['index'])) {
            $_params['smarty_internal_index'] = '[' . $_attr['index'] . ']';
            unset($_attr['index']);
        } else {
            $_params['smarty_internal_index'] = '[]';
        }
        $_new_attr = array();
        foreach ($_attr as $key => $value) {
            $_new_attr[] = array($key => $value);
        }
        // call compile assign
        return parent::compile($_new_attr, $compiler, $_params);
    }

}

?>
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

   Source File Name = smarty_internal_compile_setfilter.php

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
 * Smarty Internal Plugin Compile Setfilter
 *
 * Compiles code for setfilter tag
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Setfilter Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Setfilter extends Smarty_Internal_CompileBase {

    /**
     * Compiles code for setfilter tag
     *
     * @param array  $args      array with attributes from parser
     * @param object $compiler  compiler object
     * @param array  $parameter array with compilation parameter
     * @return string compiled code
     */
    public function compile($args, $compiler, $parameter)
    {
        $compiler->variable_filter_stack[] = $compiler->template->variable_filters;
        $compiler->template->variable_filters = $parameter['modifier_list'];
        // this tag does not return compiled code
        $compiler->has_code = false;
        return true;
    }

}

/**
 * Smarty Internal Plugin Compile Setfilterclose Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Setfilterclose extends Smarty_Internal_CompileBase {

    /**
     * Compiles code for the {/setfilter} tag
     *
     * This tag does not generate compiled output. It resets variable filter.
     *
     * @param array  $args     array with attributes from parser
     * @param object $compiler compiler object
     * @return string compiled code
     */
    public function compile($args, $compiler)
    {
        $_attr = $this->getAttributes($compiler, $args);
        // reset variable filter to previous state
        if (count($compiler->variable_filter_stack)) {
            $compiler->template->variable_filters = array_pop($compiler->variable_filter_stack);
        } else {
            $compiler->template->variable_filters = array();
        }
        // this tag does not return compiled code
        $compiler->has_code = false;
        return true;
    }

}

?>
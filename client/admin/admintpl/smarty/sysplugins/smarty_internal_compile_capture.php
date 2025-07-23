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

   Source File Name = smarty_internal_compile_capture.php

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
 * Smarty Internal Plugin Compile Capture
 *
 * Compiles the {capture} tag
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Capture Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Capture extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $shorttag_order = array('name');
    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('name', 'assign', 'append');

    /**
     * Compiles code for the {capture} tag
     *
     * @param array  $args     array with attributes from parser
     * @param object $compiler compiler object
     * @return string compiled code
     */
    public function compile($args, $compiler)
    {
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);

        $buffer = isset($_attr['name']) ? $_attr['name'] : "'default'";
        $assign = isset($_attr['assign']) ? $_attr['assign'] : 'null';
        $append = isset($_attr['append']) ? $_attr['append'] : 'null';

        $compiler->_capture_stack[0][] = array($buffer, $assign, $append, $compiler->nocache);
        // maybe nocache because of nocache variables
        $compiler->nocache = $compiler->nocache | $compiler->tag_nocache;
        $_output = "<?php \$_smarty_tpl->_capture_stack[0][] = array($buffer, $assign, $append); ob_start(); ?>";

        return $_output;
    }

}

/**
 * Smarty Internal Plugin Compile Captureclose Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_CaptureClose extends Smarty_Internal_CompileBase {

    /**
     * Compiles code for the {/capture} tag
     *
     * @param array  $args     array with attributes from parser
     * @param object $compiler compiler object
     * @return string compiled code
     */
    public function compile($args, $compiler)
    {
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);
        // must endblock be nocache?
        if ($compiler->nocache) {
            $compiler->tag_nocache = true;
        }

        list($buffer, $assign, $append, $compiler->nocache) = array_pop($compiler->_capture_stack[0]);

        $_output = "<?php list(\$_capture_buffer, \$_capture_assign, \$_capture_append) = array_pop(\$_smarty_tpl->_capture_stack[0]);\n";
        $_output .= "if (!empty(\$_capture_buffer)) {\n";
        $_output .= " if (isset(\$_capture_assign)) \$_smarty_tpl->assign(\$_capture_assign, ob_get_contents());\n";
        $_output .= " if (isset( \$_capture_append)) \$_smarty_tpl->append( \$_capture_append, ob_get_contents());\n";
        $_output .= " Smarty::\$_smarty_vars['capture'][\$_capture_buffer]=ob_get_clean();\n";
        $_output .= "} else \$_smarty_tpl->capture_error();?>";
        return $_output;
    }

}

?>
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

   Source File Name = smarty_internal_compile_private_block_plugin.php

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
 * Smarty Internal Plugin Compile Block Plugin
 *
 * Compiles code for the execution of block plugin
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Block Plugin Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Private_Block_Plugin extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('_any');

    /**
     * Compiles code for the execution of block plugin
     *
     * @param array  $args      array with attributes from parser
     * @param object $compiler  compiler object
     * @param array  $parameter array with compilation parameter
     * @param string $tag       name of block plugin
     * @param string $function  PHP function name
     * @return string compiled code
     */
    public function compile($args, $compiler, $parameter, $tag, $function)
    {
        if (!isset($tag[5]) || substr($tag, -5) != 'close') {
            // opening tag of block plugin
            // check and get attributes
            $_attr = $this->getAttributes($compiler, $args);
            if ($_attr['nocache'] === true) {
                $compiler->tag_nocache = true;
            }
               unset($_attr['nocache']);
            // convert attributes into parameter array string
            $_paramsArray = array();
            foreach ($_attr as $_key => $_value) {
                if (is_int($_key)) {
                    $_paramsArray[] = "$_key=>$_value";
                } else {
                    $_paramsArray[] = "'$_key'=>$_value";
                }
            }
            $_params = 'array(' . implode(",", $_paramsArray) . ')';

            $this->openTag($compiler, $tag, array($_params, $compiler->nocache));
            // maybe nocache because of nocache variables or nocache plugin
            $compiler->nocache = $compiler->nocache | $compiler->tag_nocache;
            // compile code
            $output = "<?php \$_smarty_tpl->smarty->_tag_stack[] = array('{$tag}', {$_params}); \$_block_repeat=true; echo {$function}({$_params}, null, \$_smarty_tpl, \$_block_repeat);while (\$_block_repeat) { ob_start();?>";
        } else {
            // must endblock be nocache?
            if ($compiler->nocache) {
                $compiler->tag_nocache = true;
            }
            // closing tag of block plugin, restore nocache
            list($_params, $compiler->nocache) = $this->closeTag($compiler, substr($tag, 0, -5));
            // This tag does create output
            $compiler->has_output = true;
            // compile code
            if (!isset($parameter['modifier_list'])) {
                $mod_pre = $mod_post ='';
            } else {
                $mod_pre = ' ob_start(); ';
                $mod_post = 'echo '.$compiler->compileTag('private_modifier',array(),array('modifierlist'=>$parameter['modifier_list'],'value'=>'ob_get_clean()')).';';
            }
            $output = "<?php \$_block_content = ob_get_clean(); \$_block_repeat=false;".$mod_pre." echo {$function}({$_params}, \$_block_content, \$_smarty_tpl, \$_block_repeat); ".$mod_post." } array_pop(\$_smarty_tpl->smarty->_tag_stack);?>";
        }
        return $output . "\n";
    }

}

?>
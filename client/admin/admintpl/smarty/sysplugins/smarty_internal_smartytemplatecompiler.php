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

   Source File Name = smarty_internal_smartytemplatecompiler.php

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
 * Smarty Internal Plugin Smarty Template Compiler Base
 *
 * This file contains the basic classes and methodes for compiling Smarty templates with lexer/parser
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * @ignore
 */
include ("smarty_internal_parsetree.php");

/**
 * Class SmartyTemplateCompiler
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_SmartyTemplateCompiler extends Smarty_Internal_TemplateCompilerBase {

    /**
     * Lexer class name
     *
     * @var string
     */
    public $lexer_class;

    /**
     * Parser class name
     *
     * @var string
     */
    public $parser_class;

    /**
     * Lexer object
     *
     * @var object
     */
    public $lex;

    /**
     * Parser object
     *
     * @var object
     */
    public $parser;

    /**
     * Smarty object
     *
     * @var object
     */
    public $smarty;

    /**
     * array of vars which can be compiled in local scope
     *
     * @var array
     */
    public $local_var = array();

    /**
     * Initialize compiler
     *
     * @param string $lexer_class  class name
     * @param string $parser_class class name
     * @param Smarty $smarty       global instance
     */
    public function __construct($lexer_class, $parser_class, $smarty)
    {
        $this->smarty = $smarty;
        parent::__construct();
        // get required plugins
        $this->lexer_class = $lexer_class;
        $this->parser_class = $parser_class;
    }

    /**
     * Methode to compile a Smarty template
     *
     * @param  mixed $_content template source
     * @return bool true if compiling succeeded, false if it failed
     */
    protected function doCompile($_content)
    {
        /* here is where the compiling takes place. Smarty
          tags in the templates are replaces with PHP code,
          then written to compiled files. */
        // init the lexer/parser to compile the template
        $this->lex = new $this->lexer_class($_content, $this);
        $this->parser = new $this->parser_class($this->lex, $this);
        if ($this->smarty->_parserdebug)
            $this->parser->PrintTrace();
        // get tokens from lexer and parse them
        while ($this->lex->yylex() && !$this->abort_and_recompile) {
            if ($this->smarty->_parserdebug) {
                echo "<pre>Line {$this->lex->line} Parsing  {$this->parser->yyTokenName[$this->lex->token]} Token " .
                    htmlentities($this->lex->value) . "</pre>";
            }
            $this->parser->doParse($this->lex->token, $this->lex->value);
        }

        if ($this->abort_and_recompile) {
            // exit here on abort
            return false;
        }
        // finish parsing process
        $this->parser->doParse(0, 0);
        // check for unclosed tags
        if (count($this->_tag_stack) > 0) {
            // get stacked info
            list($openTag, $_data) = array_pop($this->_tag_stack);
            $this->trigger_template_error("unclosed {" . $openTag . "} tag");
        }
        // return compiled code
        // return str_replace(array("? >\n<?php","? ><?php"), array('',''), $this->parser->retvalue);
        return $this->parser->retvalue;
    }

}

?>
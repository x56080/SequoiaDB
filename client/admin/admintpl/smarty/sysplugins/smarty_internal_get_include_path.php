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

   Source File Name = smarty_internal_get_include_path.php

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
 * Smarty read include path plugin
 *
 * @package Smarty
 * @subpackage PluginsInternal
 * @author Monte Ohrt
 */

/**
 * Smarty Internal Read Include Path Class
 *
 * @package Smarty
 * @subpackage PluginsInternal
 */
class Smarty_Internal_Get_Include_Path {

    /**
     * Return full file path from PHP include_path
     *
     * @param string $filepath filepath
     * @return string|boolean full filepath or false
     */
    public static function getIncludePath($filepath)
    {
        static $_include_path = null;
        
        if (function_exists('stream_resolve_include_path')) {
            // available since PHP 5.3.2
            return stream_resolve_include_path($filepath);
        }

        if ($_include_path === null) {
            $_include_path = explode(PATH_SEPARATOR, get_include_path());
        }

        foreach ($_include_path as $_path) {
            if (file_exists($_path . DS . $filepath)) {
                return $_path . DS . $filepath;
            }
        }

        return false;
    }

}

?>
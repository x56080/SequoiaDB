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

   Source File Name = smarty_internal_write_file.php

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
 * Smarty write file plugin
 *
 * @package Smarty
 * @subpackage PluginsInternal
 * @author Monte Ohrt
 */

/**
 * Smarty Internal Write File Class
 *
 * @package Smarty
 * @subpackage PluginsInternal
 */
class Smarty_Internal_Write_File {

    /**
     * Writes file in a safe way to disk
     *
     * @param string $_filepath complete filepath
     * @param string $_contents file content
     * @param Smarty $smarty    smarty instance
     * @return boolean true
     */
    public static function writeFile($_filepath, $_contents, Smarty $smarty)
    {
        $_error_reporting = error_reporting();
        error_reporting($_error_reporting & ~E_NOTICE & ~E_WARNING);
        if ($smarty->_file_perms !== null) {
            $old_umask = umask(0);
        }

        $_dirpath = dirname($_filepath);
        // if subdirs, create dir structure
        if ($_dirpath !== '.' && !file_exists($_dirpath)) {
            mkdir($_dirpath, $smarty->_dir_perms === null ? 0777 : $smarty->_dir_perms, true);
        }

        // write to tmp file, then move to overt file lock race condition
        $_tmp_file = $_dirpath . DS . uniqid('wrt', true);
        if (!file_put_contents($_tmp_file, $_contents)) {
            error_reporting($_error_reporting);
            throw new SmartyException("unable to write file {$_tmp_file}");
            return false;
        }
        
        /*
         * Windows' rename() fails if the destination exists,
         * Linux' rename() properly handles the overwrite.
         * Simply unlink()ing a file might cause other processes 
         * currently reading that file to fail, but linux' rename()
         * seems to be smart enough to handle that for us.
         */
        if (Smarty::$_IS_WINDOWS) {
            // remove original file
            @unlink($_filepath);
            // rename tmp file
            $success = @rename($_tmp_file, $_filepath);
        } else {
            // rename tmp file
            $success = @rename($_tmp_file, $_filepath);
            if (!$success) {
                // remove original file
                @unlink($_filepath);
                // rename tmp file
                $success = @rename($_tmp_file, $_filepath);
            }
        }

        if (!$success) {
            error_reporting($_error_reporting);
            throw new SmartyException("unable to write file {$_filepath}");
            return false;
        }

        if ($smarty->_file_perms !== null) {
            // set file permissions
            chmod($_filepath, $smarty->_file_perms);
            umask($old_umask);
        }
        error_reporting($_error_reporting);
        return true;
    }

}

?>
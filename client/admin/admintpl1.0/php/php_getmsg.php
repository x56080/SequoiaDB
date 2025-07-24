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

   Source File Name = php_getmsg.php

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
if (!session_id()) session_start();
$order = empty( $_POST['order'] ) ? "" : $_POST['order'] ;
if ( $order == "resolution" )
{
	$_SESSION['width'] = empty( $_POST['width'] ) ? 1300 : $_POST['width'] ;
	$_SESSION['height'] = empty( $_POST['height'] ) ? 800 : $_POST['height'] ;
	echo $_SESSION['width'].",".$_SESSION['height'] ;
}
else
{
	echo "unknow";
}
?>
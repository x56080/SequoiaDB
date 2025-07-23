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

   Source File Name = index.php

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
session_start() ;
date_default_timezone_set ( "UTC" ) ;
set_time_limit( 0 ) ;

//初始化常量
require_once ( "./php/html_conf.php" ) ;

//初始化php模板
require_once ( "./php/sdb-init.php" ) ;

//取得错误映射表
require_once ( "./php/error_cn.php" ) ;

//进入网页显示
require_once ( './php/html-show.php' ) ;

?>
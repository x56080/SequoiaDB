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

   Source File Name = a-monitor_l.php

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

$isfirst = true ;
$group_list = '' ;
$db -> install ( "{ install : false }" ) ;
$cursor = $db -> getList ( SDB_LIST_GROUPS ) ;
if ( !empty ( $cursor ) )
{
	$group_list = '{"name":"数据库","child":{"name1":"数据统计","name2":"性能监控","name3":"分区组","child":[' ;
	while ( $arr = $cursor -> getNext() )
	{
		if ( !$isfirst )
		{
			$group_list .= "," ;
		}
		else
		{
			$isfirst = false ;
		}
		$group_list .= $arr ;
	}
	$group_list .= ']}}' ;
}
else
{
	$group_list = '{"name":"数据库","child":{"name1":"数据统计","name2":"性能监控"}}' ;
}
$db -> install ( "{ install : true }" ) ;

$smarty -> assign( "group_list", $group_list ) ;

?>
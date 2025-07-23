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

   Source File Name = a-group_r.php

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

$groupname  = empty ( $_POST['group'] ) ? "" : $_POST['group'] ;
$hostname  = empty ( $_POST['hostname'] ) ? "" : $_POST['hostname'] ;
$nodename   = empty ( $_POST['node'] )  ? "" : $_POST['node'] ;

$isgroupnode = 0 ;
$group_list = array() ;
$node_list = array() ;

if ( $groupname != "" && $nodename == "" )
{
	$isgroupnode = 1 ;
	$cursor = $db -> getList ( SDB_LIST_GROUPS, '{GroupName:"'.$groupname.'"}' ) ;
	if ( !empty ( $cursor ) )
	{
		while ( $arr = $cursor -> getNext() )
		{
			$group_list = $arr ;
		}
	}
}
else if ( $groupname != "" && $nodename != "" )
{
	$isgroupnode = 2 ;
	$cursor = $db -> getList ( SDB_LIST_GROUPS, '{GroupName:"'.$groupname.'"}', '{Group:1}' ) ;
	if ( !empty ( $cursor ) )
	{
		while ( $arr = $cursor -> getNext() )
		{
			foreach ( $arr["Group"] as $child )
			{
				if ( $child["HostName"] == $hostname )
				{
					foreach ( $child["Service"] as $child_child )
					{
						if ( $nodename == $child_child["Name"] && $child_child["Type"] == 0 )
						{
							$node_list = $child ;
						}
					}
				}
			}
		}
	}
}

$smarty -> assign( "isgroupnode" , $isgroupnode ) ;
$smarty -> assign( "group_list" , $group_list ) ;
$smarty -> assign( "node_list" , $node_list ) ;

?>
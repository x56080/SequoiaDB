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

   Source File Name = a-group_w.php

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

$type        = empty ( $_POST['type']        ) ? ""   :  $_POST['type']        ;
$groupname   = empty ( $_POST['groupname']   ) ? ""   :  $_POST['groupname']   ;
$nodename    = empty ( $_POST['nodename']    ) ? ""   :  $_POST['nodename']    ;
$hostname    = empty ( $_POST['hostname']    ) ? ""   :  $_POST['hostname']    ;
$serivcename = empty ( $_POST['serivcename'] ) ? ""   :  $_POST['serivcename'] ;
$databasepath= empty ( $_POST['databasepath']) ? ""   :  $_POST['databasepath'];
$config      = empty ( $_POST['config']      ) ? NULL :  $_POST['config']      ;

$rc = 0 ;
if ( $type == "creategroup" )
{
	$group = $db -> createGroup ( $groupname ) ;
	if ( empty ( $group ) )
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "createcatalog" )
{
	$arr = $db -> createCataGroup ( $hostname, $serivcename, $databasepath, $config ) ;
	$rc = $arr["errno"] ;
}
else if ( $type == "createnode" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$arr = $group -> createNode ( $hostname, $serivcename, $databasepath, $config ) ;
		$rc = $arr["errno"] ;
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "startgroup" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$arr = $group -> start() ;
		$rc = $arr["errno"] ;
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "stopgroup" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$arr = $group -> stop() ;
		$rc = $arr["errno"] ;
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "startnode" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$node = $group -> getNode( $nodename ) ;
		if ( !empty ( $node ) )
		{
			$arr = $node -> start() ;
			$rc = $arr["errno"] ;
		}
		else
		{
			$arr = $db -> getError() ;
			$rc = $arr["errno"] ;
		}
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "stopnode" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$node = $group -> getNode( $nodename ) ;
		if ( !empty ( $node ) )
		{
			$arr = $node -> stop() ;
			$rc = $arr["errno"] ;
		}
		else
		{
			$arr = $db -> getError() ;
			$rc = $arr["errno"] ;
		}
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}

$smarty -> assign( "group_type", $type ) ;
$smarty -> assign( "group_rc", $rc ) ;
$smarty -> assign( "group_respond", array_key_exists( $rc, $errno_cn ) ? $errno_cn[$rc] : $rc ) ;
?>
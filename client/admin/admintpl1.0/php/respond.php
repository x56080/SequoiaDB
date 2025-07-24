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

   Source File Name = respond.php

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
include_once ( "php_lib.php" ) ;
if ( empty($_SESSION['address']) )
{
	exit();
}
ini_set ( 'memory_limit', "1024M" ) ;
$width = $_SESSION['width'];
$height = $_SESSION['height'];
$db = new SequoiaDB() ;
$db->connect ( $_SESSION['address'], $_SESSION['user'], $_SESSION['password'] ) ;
$arr = $db->getError () ;
if ( $arr['errno'] )
{
	exit() ;
}
$insert = 0 ;
$delete = 0 ;
$update = 0 ;
$insert_c = 0 ;
$delete_c = 0 ;
$update_c = 0 ;
$query = 0 ;
$memory = 0 ;
$disk = 0 ;
$mmemory = 0 ;
$mdisk = 0 ;
$model = 'unknown' ;
$count_node = 0 ;
if ( !empty ( $_POST['order'] ) )
{
	if ( $_POST['order'] == 'getMaxMAD' )
	{
		if ( !empty ( $_POST['model'] ) )
		{
			$model = $_POST['model'] ;
			if ( $model == 'all' )
			{
				$cursor = $db->getList ( SDB_LIST_GROUPS ) ;
				$str = "" ;
				if ( !empty( $cursor ) )
				{
					$phy_arr = array() ;
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						++$count_node ;
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) )
							{
								$arr_1 = $cursor1->getNext() ;
								if ( !in_array ( $arr_1['HostName'], $phy_arr ) )
								{
									array_push ( $phy_arr, $arr_1['HostName'] ) ;
									$temp_arr = $arr_1['Memory'] ;
									$memory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
									$temp_arr = $arr_1['Disk'] ;
									$disk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
								}
							}
						}
					}
				}
				if ( $count_node == 0 )
				{
					$cursor1 = $db->getSnapshot ( SDB_SNAP_SYSTEM ) ;
					if ( !empty($cursor1) )
					{
						$arr_1 = $cursor1->getNext() ;
						if ( !in_array ( $arr_1['HostName'], $phy_arr ) )
						{
							array_push ( $phy_arr, $arr_1['HostName'] ) ;
							$temp_arr = $arr_1['Memory'] ;
							$memory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
							$temp_arr = $arr_1['Disk'] ;
							$disk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
						}
					}
				}
				echo '{MaxMemory:'.$memory.',MaxDisk:'.$disk.'}' ;
				exit();
			}
			else if ( $model == 'group' )
			{
				$GroupName = empty($_POST['GroupName']) ? "" : $_POST['GroupName'] ;
				$cursor = $db->getList ( SDB_LIST_GROUPS, NULL ) ;
				$str = "" ;
				if ( !empty( $cursor ) )
				{
					$phy_arr = array() ;
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) )
							{
								$arr_1 = $cursor1->getNext() ;
								if ( !in_array ( $arr_1['HostName'], $phy_arr ) && $GroupName == $arr_1['GroupName'] )
								{
									array_push ( $phy_arr, $arr_1['HostName'] ) ;
									$temp_arr = $arr_1['Memory'] ;
									$memory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
									$temp_arr = $arr_1['Disk'] ;
									$disk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
								}
							}
						}
					}
				}
				echo '{MaxMemory:'.$memory.',MaxDisk:'.$disk.'}' ;
				exit();
			}
			else if ( $model == 'phy' )
			{
				$HostName = empty($_POST['HostName']) ? "" : $_POST['HostName'] ;
				$cursor = $db->getList ( SDB_LIST_GROUPS, NULL ) ;
				$str = "" ;
				if ( !empty( $cursor ) )
				{
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) )
							{
								$arr_1 = $cursor1->getNext() ;
								if ( $HostName == $arr_1['HostName'] )
								{
									$temp_arr = $arr_1['Memory'] ;
									$memory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
									$temp_arr = $arr_1['Disk'] ;
									$disk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
									break 2 ;
								}
							}
						}
					}
				}
				echo '{MaxMemory:'.$memory.',MaxDisk:'.$disk.'}' ;
				exit();
			}
			else if ( $model == 'node' )
			{
				$NodeName = empty($_POST['NodeName']) ? "" : $_POST['NodeName'] ;
				$cursor = $db->getList ( SDB_LIST_GROUPS, NULL ) ;
				$str = "" ;
				if ( !empty( $cursor ) )
				{
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) )
							{
								$arr_1 = $cursor1->getNext() ;
								if ( $NodeName == $arr_1['NodeName'] )
								{
									$temp_arr = $arr_1['Memory'] ;
									$memory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
									$temp_arr = $arr_1['Disk'] ;
									$disk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
									break 2 ;
								}
							}
						}
					}
				}
				echo '{MaxMemory:'.$memory.',MaxDisk:'.$disk.'}' ;
				exit();
			}
			else
			{
				$model = 'unknown' ;
				echo '{MaxMemory:0,MaxDisk:0}' ;
				exit();
			}
		}
	}
	else if ( $_POST['order'] == 'getData' )
	{
		if ( !empty ( $_POST['model'] ) )
		{
			$model = $_POST['model'] ;
			if ( $model == 'all' )
			{
				$cursor = $db->getList ( SDB_LIST_GROUPS ) ;
				if ( !empty( $cursor ) )
				{
					$phy_arr = array () ;
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						++$count_node ;
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE, $str ) ;
							$cursor2 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) && !empty($cursor2) )
							{
								$arr_1 = $cursor1->getNext() ;
								$arr_2 = $cursor2->getNext() ;
								//if( $arr_1['Role'] != 'catalog' )
								if ( !empty($arr_1) && !empty($arr_2) )
								{
									$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
									$insert_c += getNumber($arr_1['ReplInsert']) ;
									$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
									$delete_c += getNumber($arr_1['ReplDelete']) ;
									$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
									$update_c += getNumber($arr_1['ReplUpdate']) ;
									$query += getNumber($arr_1['TotalRead']) ;
									if ( !in_array ( $arr_1['HostName'], $phy_arr ) )
									{
										array_push ( $phy_arr, $arr_1['HostName'] ) ;
										$temp_arr = $arr_2['Memory'] ;
										$memory +=(int)((getNumber($temp_arr['TotalRAM'])-getNumber($temp_arr['FreeRAM']))/1048576) ;
										$mmemory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
										$temp_arr = $arr_2['Disk'] ;
										$disk += (int)((getNumber($temp_arr['TotalSpace'])-getNumber($temp_arr['FreeSpace']))/1073741824) ;
										$mdisk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
									}
								}
							}
						}
					}
				}
				if ( $count_node == 0 )
				{
					$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE ) ;
					$cursor2 = $db->getSnapshot ( SDB_SNAP_SYSTEM ) ;
					if ( !empty($cursor1) && !empty($cursor2) )
					{
						$arr_1 = $cursor1->getNext() ;
						$arr_2 = $cursor2->getNext() ;
						if ( !empty($arr_1) && !empty($arr_2) )
						{
							$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
							$insert_c += getNumber($arr_1['ReplInsert']) ;
							$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
							$delete_c += getNumber($arr_1['ReplDelete']) ;
							$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
							$update_c += getNumber($arr_1['ReplUpdate']) ;
							$query += getNumber($arr_1['TotalRead']) ;
							
							if ( !in_array ( $arr_1['HostName'], $phy_arr ) )
							{
								array_push ( $phy_arr, $arr_1['HostName'] ) ;
								$temp_arr = $arr_2['Memory'] ;
								$memory += (int)((getNumber($temp_arr['TotalRAM'])-getNumber($temp_arr['FreeRAM']))/1048576) ;
								$mmemory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
								$temp_arr = $arr_2['Disk'] ;
								$disk += (int)((getNumber($temp_arr['TotalSpace'])-getNumber($temp_arr['FreeSpace']))/1073741824) ;
								$mdisk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
							}
						}
					}
				}
				echo '{insert:'.$insert.',del:'.$delete.',update:'.$update.',insertc:'.$insert_c.',deletec:'.$delete_c.',updatec:'.$update_c.',query:'.$query.',memory:'.$memory.',disk:'.$disk.',tmemory:'.$mmemory.',tdisk:'.$mdisk.'}' ;
				exit();
			}
			else if ( $model == 'group' )
			{
				$GroupName = empty($_POST['GroupName']) ? "" : $_POST['GroupName'] ;
				$cursor = $db->getList ( SDB_LIST_GROUPS, NULL ) ;
				$str = "" ;
				if ( !empty( $cursor ) )
				{
					$phy_arr = array () ;
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE, $str ) ;
							$cursor2 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) && !empty($cursor2) )
							{
								$arr_1 = $cursor1->getNext() ;
								$arr_2 = $cursor2->getNext() ;
								if ( $GroupName == $arr_1['GroupName'] )
								{
									$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
									$insert_c += getNumber($arr_1['ReplInsert']) ;
									$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
									$delete_c += getNumber($arr_1['ReplDelete']) ;
									$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
									$update_c += getNumber($arr_1['ReplUpdate']) ;
									$query += getNumber($arr_1['TotalRead']) ;
									
									if ( !in_array ( $arr_1['HostName'], $phy_arr ) )
									{
										array_push ( $phy_arr, $arr_1['HostName'] ) ;
										$temp_arr = $arr_2['Memory'] ;
										$memory +=(int)((getNumber($temp_arr['TotalRAM'])-getNumber($temp_arr['FreeRAM']))/1048576) ;
										$mmemory +=(int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
										$temp_arr = $arr_2['Disk'] ;
										$disk += (int)((getNumber($temp_arr['TotalSpace'])-getNumber($temp_arr['FreeSpace']))/1073741824) ;
										$mdisk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
									}
								}
							}
						}
					}
				}
				echo '{insert:'.$insert.',del:'.$delete.',update:'.$update.',insertc:'.$insert_c.',deletec:'.$delete_c.',updatec:'.$update_c.',query:'.$query.',memory:'.$memory.',disk:'.$disk.',tmemory:'.$mmemory.',tdisk:'.$mdisk.'}' ;
				exit() ;
			}
			else if ( $model == 'phy' )
			{
				$HostName = empty($_POST['HostName']) ? "" : $_POST['HostName'] ;
				$cursor = $db->getList ( SDB_LIST_GROUPS, NULL ) ;
				$str = "" ;
				if ( !empty( $cursor ) )
				{
					$phy_arr = array();
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE, $str ) ;
							$cursor2 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) && !empty($cursor2) )
							{
								$arr_1 = $cursor1->getNext() ;
								$arr_2 = $cursor2->getNext() ;
								if ( $HostName == $arr_1['HostName'] )//&& $arr_1['Role'] != "catalog" )
								{
									$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
									$insert_c += getNumber($arr_1['ReplInsert']) ;
									$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
									$delete_c += getNumber($arr_1['ReplDelete']) ;
									$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
									$update_c += getNumber($arr_1['ReplUpdate']) ;
									$query += getNumber($arr_1['TotalRead']) ;
									
									if ( !in_array ( $arr_1['HostName'], $phy_arr ) )
									{
										array_push ( $phy_arr, $arr_1['HostName'] ) ;
										$temp_arr = $arr_2['Memory'] ;
										$memory +=(int)((getNumber($temp_arr['TotalRAM'])-getNumber($temp_arr['FreeRAM']))/1048576) ;
										$mmemory +=(int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
										$temp_arr = $arr_2['Disk'] ;
										$disk += (int)((getNumber($temp_arr['TotalSpace'])-getNumber($temp_arr['FreeSpace']))/1073741824) ;
										$mdisk += (int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
									}
								}
							}
						}
					}
				}
				echo '{insert:'.$insert.',del:'.$delete.',update:'.$update.',insertc:'.$insert_c.',deletec:'.$delete_c.',updatec:'.$update_c.',query:'.$query.',memory:'.$memory.',disk:'.$disk.',tmemory:'.$mmemory.',tdisk:'.$mdisk.'}' ;
				exit();
			}
			else if ( $model == 'node' )
			{
				$nodename = empty($_POST['NodeName']) ? "" : $_POST['NodeName'] ;
				$cursor = $db->getList ( SDB_LIST_GROUPS, NULL ) ;
				$str = "" ;
				if ( !empty( $cursor ) )
				{
					while ( $arr = $cursor->getNext() )
					{
						if ( array_key_exists ( "Status", $arr ) )
						{
							if ( !$arr['Status'] )
							{
								continue ;
							}
						}
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
							$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE, $str ) ;
							$cursor2 = $db->getSnapshot ( SDB_SNAP_SYSTEM, $str ) ;
							if ( !empty($cursor1) && !empty($cursor2) )
							{
								$arr_1 = $cursor1->getNext() ;
								$arr_2 = $cursor2->getNext() ;
								if ( $nodename == $arr_1['NodeName'] )
								{
									$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
									$insert_c += getNumber($arr_1['ReplInsert']) ;
									$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
									$delete_c += getNumber($arr_1['ReplDelete']) ;
									$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
									$update_c += getNumber($arr_1['ReplUpdate']) ;
									$query += getNumber($arr_1['TotalRead']) ;
									$temp_arr = $arr_2['Memory'] ;
									$memory += (int)((getNumber($temp_arr['TotalRAM'])-getNumber($temp_arr['FreeRAM']))/1048576) ;
									$mmemory += (int)(getNumber($temp_arr['TotalRAM'])/1048576) ;
									$temp_arr = $arr_2['Disk'] ;
									$disk+=(int)((getNumber($temp_arr['TotalSpace'])-getNumber($temp_arr['FreeSpace']))/1073741824) ;
									$mdisk+=(int)(getNumber($temp_arr['TotalSpace'])/1073741824) ;
									break 2 ;
								}
							}
						}
					}
				}
				echo '{insert:'.$insert.',del:'.$delete.',update:'.$update.',insertc:'.$insert_c.',deletec:'.$delete_c.',updatec:'.$update_c.',query:'.$query.',memory:'.$memory.',disk:'.$disk.',tmemory:'.$mmemory.',tdisk:'.$mdisk.'}' ;
				exit();
			}
			else
			{
				$model = 'unknown' ;
				echo '{insert:0,del:0,update:0,insertc:0,deletec:0,updatec:0,query:0,memory:0,disk:0}' ;
				exit();
			}
		}
		echo '{insert:0,del:0,update:0,insertc:0,deletec:0,updatec:0,query:0,memory:0,disk:0}' ;
	}
}
?>
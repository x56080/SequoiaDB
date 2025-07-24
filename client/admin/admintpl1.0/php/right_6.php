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

   Source File Name = right_6.php

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
if ( empty($_SESSION['address']) || empty($_SESSION['width']) || empty($_SESSION['height']) )
{
	echo '<script type="text/javascript" language="javascript">window.parent.location.href="../login.php"; </script>' ;
	exit();
}
$width = $_SESSION['width'];
$height = $_SESSION['height'];
$db = new SequoiaDB() ;
$db->connect ( $_SESSION['address'], $_SESSION['user'], $_SESSION['password'] ) ;
$arr = $db->getError () ;
if ( $arr['errno'] )
{
	echo '<script type="text/javascript" language="javascript">window.parent.location.href="../login.php"; </script>' ;
	exit() ;
}
$insert = 0 ;
$delete = 0 ;
$update = 0 ;
$insert_c = 0 ;
$delete_c = 0 ;
$update_c = 0 ;
$query = 0 ;
$model = 'unknown' ;
$GroupName = "";
$HostName = "" ;
$nodename = "" ;
$count_node = 0 ;
if ( !empty ( $_GET['model'] ) )
{
	$model = $_GET['model'] ;
	if ( $model == 'all' )
	{
		$cursor = $db->getList ( SDB_LIST_GROUPS ) ;
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
				++$count_node ;
				$child_arr = $arr['Group'] ;
				while ( list( $key, $val ) = each ( $child_arr ) )
				{
					$arr2 = $child_arr[$key] ;
					$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
					$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE, $str ) ;
					if ( !empty($cursor1) )
					{
						$arr_1 = $cursor1->getNext() ;
						//if( $arr_1['Role'] != 'catalog' )
						if ( !empty($arr_1) )
						{
							$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
							$insert_c += getNumber($arr_1['ReplInsert']) ;
							$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
							$delete_c += getNumber($arr_1['ReplDelete']) ;
							$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
							$update_c += getNumber($arr_1['ReplUpdate']) ;
							$query += getNumber($arr_1['TotalRead']) ;
						}
					}
				}
			}
			if ( $count_node == 0 )
			{
				$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE ) ;
				if ( !empty($cursor1) )
				{
					$arr_1 = $cursor1->getNext() ;
					if ( !empty($arr_1) )
					{
						$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
						$insert_c += getNumber($arr_1['ReplInsert']) ;
						$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
						$delete_c += getNumber($arr_1['ReplDelete']) ;
						$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
						$update_c += getNumber($arr_1['ReplUpdate']) ;
						$query += getNumber($arr_1['TotalRead']) ;
					}
				}
			}
		}
	}
	else if ( $model == 'group' )
	{
		$GroupName = empty($_GET['GroupName']) ? "" : $_GET['GroupName'] ;
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
					if ( !empty($cursor1) )
					{
						$arr_1 = $cursor1->getNext() ;
						if ( $GroupName == $arr_1['GroupName'] )
						{
							$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
							$insert_c += getNumber($arr_1['ReplInsert']) ;
							$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
							$delete_c += getNumber($arr_1['ReplDelete']) ;
							$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
							$update_c += getNumber($arr_1['ReplUpdate']) ;
							$query += getNumber($arr_1['TotalRead']) ;
						}
					}
				}
			}
		}
	}
	else if ( $model == 'phy' )
	{
		$HostName = empty($_GET['HostName']) ? "" : $_GET['HostName'] ;
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
					if ( !empty($cursor1) )
					{
						$arr_1 = $cursor1->getNext() ;
						if ( $HostName == $arr_1['HostName'] )//&& $arr_1['Role'] != "catalog" )
						{
							$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
							$insert_c += getNumber($arr_1['ReplInsert']) ;
							$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
							$delete_c += getNumber($arr_1['ReplDelete']) ;
							$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
							$update_c += getNumber($arr_1['ReplUpdate']) ;
							$query += getNumber($arr_1['TotalRead']) ;
						}
					}
				}
			}
		}
	}
	else if ( $model == 'node' )
	{
		$nodename = empty($_GET['nodename']) ? "" : $_GET['nodename'] ;
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
					if ( !empty($cursor1) )
					{
						$arr_1 = $cursor1->getNext() ;
						if ( $nodename == $arr_1['NodeName'] )
						{
							$insert += (getNumber($arr_1['TotalInsert']) - getNumber($arr_1['ReplInsert'])) ;
							$insert_c += getNumber($arr_1['ReplInsert']) ;
							$delete += (getNumber($arr_1['TotalDelete']) -  getNumber($arr_1['ReplDelete'])) ;
							$delete_c += getNumber($arr_1['ReplDelete']) ;
							$update += (getNumber($arr_1['TotalUpdate']) - getNumber($arr_1['ReplUpdate'])) ;
							$update_c += getNumber($arr_1['ReplUpdate']) ;
							$query += getNumber($arr_1['TotalRead']) ;
							break 2 ;
						}
					}
				}
			}
		}
	}
	else
	{
		$model = 'unknown' ;
	}
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta name="viewport" content="width=device-width" />
    
    
<title>广州巨杉软件有限公司_SequoiaDB</title>

<meta name="description" content="公司介绍"/>
<link rel="canonical" href="" />
<meta name="robots" content="noodp,noydir"/>
<meta property='og:locale' content='en_US'/>
<meta property='og:title' content='标题'/>
<meta property='og:description' content='公司介绍'/>
<meta property='og:url' content='www.sequoiadb.com'/>
<meta property='og:site_name' content='公司名字'/>
<meta property='og:type' content='article'/>
<!--                       CSS                       -->
<!-- Main Stylesheet -->
<link rel='stylesheet' href='../css/right_style.css' type='text/css'/>
<link rel="stylesheet" href="../css/table_style.css" type="text/css" media="screen" />
<link rel="stylesheet" href="../css/style_s.css" type="text/css"/>

<!--                       Javascripts                       -->
<!-- jQuery -->
<!--<script type="text/javascript" language="javascript" src="../js/jquery-1.3.2.min.js"></script>-->
<script type="text/javascript" language="javascript" src="../js/lib/FlashCanvas/bin/flashcanvas.js"></script>
<script type="text/javascript" language="javascript" src="../js/lib/FlashCanvas/bin/flashcanvas.js"></script>
<script type="text/javascript" language="javascript" src="../js/flot/excanvas.min.js"></script>
<script type="text/javascript" language="javascript" src="../js/flot/jquery.js"></script>
<script type="text/javascript" language="javascript" src="../js/common.js"></script>
<script type="text/javascript" language="javascript" src="../js/flot/jquery.flot.js"></script>
<script type="text/javascript" language="javascript" src="../js/flot/jquery.flot.pie.js"></script>
<!-- jQuery Configuration -->
<!--<script type="text/javascript" src="../js/simpla.jquery.configuration.js"></script>-->


</head>
<body style="min-width:970px;">
<!-- body -->
<!-- topNavWrapper -->
    <div id="topNavWrapper">
    <ul id="topNav">
	    <li style="list-style-type: none;">
			<div>
				<div class="toptool">
					<img src="../images/diannao.png" width="16px">&nbsp;<font class="title">巨杉软件后台管理</font>
					<img src="../images/person.png" width="16px">&nbsp;<font class="hello"><font color="#D1E9E9">管理员</font>，你好！</font>
					&nbsp;&nbsp;&nbsp;<a href="right_1.php" class="achoice" target="right"><img src="../images/home.png" width="16px">&nbsp;主页</a>
                    &nbsp;&nbsp;&nbsp;<a href="../doc/index.html" class="achoice" target="new"><img src="../images/msg.png" width="16px">&nbsp;信息中心</a>
					&nbsp;&nbsp;&nbsp;<a href="#" class="achoice" target="right"><img src="../images/totop.png" width="16px">&nbsp;顶部</a>
					&nbsp;&nbsp;&nbsp;<a href="#" class="achoice" onclick="location.reload()" target="right"><img src="../images/refresh.png" width="16px">&nbsp;刷新</a>
					&nbsp;&nbsp;&nbsp;<a href="../login.php" class="achoice" target="_parent"><img src="../images/quit.png" width="16px">&nbsp;退出</a>
				</div>
			</div>
		</li>
    </ul>
    </div>
	
<!-- /topNavWrapper -->

<!-- topbutton -->
<div class="topb">
	<div class="path">
		<a href="right_1.php" class="pathlink"><img src="../images/sever.png" width="15px">&nbsp;服务器：<?php echo $_SESSION['address'] ;?></a> --> <a href="#" class="pathlink"><img src="../images/searchfile.png" width="15px">&nbsp;数据视图</a>
	</div>
	<div class="clear"></div>

	<div class="topbutton_div">
    	<a href="right_2.php">
			<div class="topbutton">
				<img src="../images/db.png" width="15px" border="0">&nbsp;逻辑组
			</div>
		</a>
        <a href="right_3.php">
			<div class="topbutton">
				<img src="../images/sever.png" width="15px" border="0">&nbsp;物理组
			</div>
		</a>
		<a href="right_4.php">
			<div class="topbutton">
				<img src="../images/cs.png" width="15px" border="0">&nbsp;集合空间
			</div>
		</a>
		<a href="right_5.php">
			<div class="topbutton">
				<img src="../images/editfile.png" width="15px" border="0">&nbsp;SQL
			</div>
		</a>
		<a href="right_6.php?model=<?php echo $model;?>&GroupName=<?php echo $GroupName;?>&HostName=<?php echo $HostName;?>&node=<?php echo $nodename;?>">
			<div class="tbcurrent">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;数据视图
			</div>
		</a>
        <a href="right_7.php?model=<?php echo $model;?>&GroupName=<?php echo $GroupName;?>&HostName=<?php echo $HostName;?>&NodeName=<?php echo $nodename;?>">
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;实时视图
			</div>
		</a>
	</div>
</div>
<div style="width:<?php echo $width-206;?>px;margin-top:50px;">
	<div style="padding-left:15px;">
    	<div style="width:<?php echo $width-500;?>px;font-size:14px;"align="center">
        <?php
		$str = "" ;
		if ( $model == 'all' )
		{
			if ( $count_node == 0 )
			{
				$str = "单节点" ;
			}
			else
			{
				$str = "集群的" ;
			}
		}
		else if ( $model == 'group' )
		{
			$str = "逻辑组 ".$GroupName."的" ;
		}
		else if ( $model == 'phy' )
		{
			$str = "物理组 ".$HostName."的" ;
		}
		else if ( $model == 'node' )
		{
			$str = "数据节点 ".$nodename."的" ;
		}
		echo $str ;
		?>数据视图
        </div>
    	<div id="data_pic" style="width:<?php echo $width-500;?>px;height:400px;">
		<script>
            window.onload=function()
            {
                draw1 ( "#data_pic",
				        <?php echo $insert;?>,
						<?php echo $delete;?>,
						<?php echo $update;?>,
						<?php echo $insert_c;?>,
						<?php echo $delete_c;?>,
						<?php echo $update_c;?>,
						<?php echo $query;?> ) ;
            }
        </script>
        </div>
    </div>
</div>
</body>
</html>
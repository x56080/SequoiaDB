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

   Source File Name = right_group.php

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
$arr_1 = "" ;
$arr_2 = "" ;
$arr_3 = "" ;
$str = "" ;
$groupname = "" ;
if ( !empty( $_POST['create_group_name'] ) )
{
	$groupname = $_POST['create_group_name'] ;
	$db->selectGroup ( $groupname ) ;
	$arr_1 = $db->getError();
	$str = 'PHP 代码: $db->selectGroup ( "'.$groupname.'" ) ;' ;
}
else if ( !empty( $_POST['hostname'] ) )
{
	$groupname = empty( $_POST['groupname'] ) ? "" : $_POST['groupname'] ;
	$hostname = empty( $_POST['hostname'] ) ? "" : $_POST['hostname'] ;
	$servicename = empty( $_POST['servicename'] ) ? "" : $_POST['servicename'] ;
	$databasepath = empty( $_POST['databasepath'] ) ? "" : $_POST['databasepath'] ;
	$config = empty( $_POST['config'] ) ? "" : $_POST['config'] ;
	$group = $db->selectGroup ( $groupname ) ;
	$arr_2 = $db->getError();
	$str = 'PHP 代码: $group->createNode ( "'.$hostname.'", "'.$servicename.'", "'.$databasepath.'", "'.$config.'" ) ;' ;
	if ( $arr_2['errno'] == 0 )
	{
		$arr_2 = $group->createNode ( $hostname, $servicename, $databasepath, $config ) ;
	}
}
else if ( !empty( $_GET['order'] ) )
{
	$order = empty( $_GET['order'] ) ? "" : $_GET['order'] ;
	if ( $order == 'start' )
	{
		$groupname = empty( $_GET['groupname'] ) ? "" : $_GET['groupname'] ;
		$group = $db->selectGroup ( $groupname ) ;
		$group->start() ;
		$arr_3 = $db->getError () ;
		$str = 'PHP 代码: $group->start() ;' ;
	}
	if ( $order == 'stop' )
	{
		$groupname = empty( $_GET['groupname'] ) ? "" : $_GET['groupname'] ;
		$group = $db->selectGroup ( $groupname ) ;
		$group->stop() ;
		$arr_3 = $db->getError () ;
		$str = 'PHP 代码: $group->stop() ;' ;
	}
	else if ( $order == 'selectgroup' )
	{
		$groupname = empty( $_GET['groupname'] ) ? "" : $_GET['groupname'] ;
		$db->selectGroup ( $groupname ) ;
		$arr_3 = $db->getError();
		$str = 'PHP 代码: $db->selectGroup ( "'.$groupname.'" ) ;' ;
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
<script type="text/javascript" src="../js/jquery-1.3.2.min.js"></script>
<script type="text/javascript" language="javascript">
$(document).ready(function(){
  $("form").submit(function(e){
    //e.preventDefault();
    //alert("Submit prevented");
	e.fromElement.submit();
  });
});
</script>
<script type="text/javascript" language="javascript">
function group_t_p ( isstart, groupname )
{
	if ( isstart )
	{
		if ( confirm ( "确定启动逻辑组吗?" ) )
		{
			var temp = 'right_group.php?order=start&groupname=' + groupname ;
			self.location = temp ;
		}
	}
	else
	{
		if ( confirm ( "确定停止逻辑组吗?" ) )
		{
			var temp = 'right_group.php?order=stop&groupname=' + groupname ;
			self.location = temp ;
		}
	}
}
</script>
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
<?php
$sum = 0 ;
$is_start = false ;
if ( $groupname != "" )
{
	$cursor = $db->getList ( SDB_LIST_GROUPS ) ;
	while ( $arr = $cursor->getNext() )
	{
		++$sum ;
		if ( $arr['GroupName'] == $groupname )
		{
			$is_start = isStart ( $db, $arr ) ;
			break;
		}
	}
}
?>
<!-- topbutton -->
<div class="topb">
	<div class="path">
		<a href="right_1.php" class="pathlink"><img src="../images/sever.png" width="15px">&nbsp;服务器：<?php echo $_SESSION['address'] ;?></a> --> <a href="#" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;逻辑组：<?php echo $groupname;?></a>
	</div>
	<div class="clear"></div>

	<div class="topbutton_div">
    	<a href="right_2.php">
			<div class="tbcurrent">
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
		<a href="right_6.php?model=group&GroupName=<?php echo $groupname;?>">
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;数据视图
			</div>
		</a>
        <a href="right_7.php?model=group&GroupName=<?php echo $groupname;?>">
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;实时视图
			</div>
		</a>
        <?php
		if ( $is_start )
		{
		?>
			<div class="topbutton" style="cursor:pointer;" onclick="group_t_p(false,'<?php echo $groupname;?>');">
				<img src="../images/del.png" width="15px" border="0">&nbsp;停止
			</div>
        <?php
		}
		else
		{
		?>
			<div class="topbutton" style="cursor:pointer;" onclick="group_t_p(true,'<?php echo $groupname;?>');">
				<img src="../images/rightarrow.png" width="15px" border="0">&nbsp;启动
			</div>
        <?php
		}
		?>
	</div>
</div>
<?php
if ( $arr_1 != "" || $arr_2 != "" || $arr_3 != "" )
{
	if ( $arr_1  != "" )
	{
		if ( $arr_1['errno'] == 0 )
		{
			echo '<script type="text/javascript" language="javascript">window.parent.frames["left"].location.reload(); </script>' ;
			//echo '新建逻辑组成功' ;
		}
		else
		{
			$sum = 0 ;
			php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
			echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 新建逻辑组失败 错误码: '.$arr_1['errno'] ;
			php_div_e_3();
		}
	}
	else if ( $arr_2 != "" )
	{
		if ( $arr_2['errno'] == 0 )
		{
			//echo '新建数据节点成功' ;
		}
		else
		{
			$sum = 0 ;
			php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
			echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 新建数据节点失败 错误码: '.$arr_2['errno'] ;
			php_div_e_3();
		}
	}
	else if ( $arr_3 != "" )
	{
		if ( $arr_3['errno'] == 0 )
		{
			if ( $order == 'start' )
			{
				//php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
				echo '<script type="text/javascript" language="javascript">window.parent.frames["left"].location.reload(); </script>' ;
				//echo '启动逻辑组成功' ;
				//php_div_e_3();
			}
			else if ( $order == 'stop' )
			{
				//php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
				echo '<script type="text/javascript" language="javascript">window.parent.frames["left"].location.reload(); </script>' ;
				//echo '停止逻辑组成功' ;
				//php_div_e_3();
			}
			else if ( $order == 'selectgroup' )
			{
				//echo '选择逻辑组成功' ;
			}
		}
		else
		{
			$sum = 0 ;
			php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
			if ( $order == 'start' )
			{
				echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 启动逻辑组失败 错误码: '.$arr_3['errno'] ;
			}
			else if ( $order == 'stop' )
			{
				echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 停止逻组辑失败 错误码: '.$arr_3['errno'] ;
			}
			else if ( $order == 'selectgroup' )
			{
				echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 选择逻辑组失败 错误码: '.$arr_3['errno'] ;
			}
			php_div_e_3();
		}
	}
}
/*
if ( $str != "" )
{
	php_div_s_3($width-250,"background-color:#F8F8F8;margin-left:15px;");
	echo $str ;
	php_div_e_3();
}
*/
?>
<div style="width:<?php echo $width-230;?>px;margin-top:20px;margin-bottom:60px;">
	<div>
        <form action="right_group.php" method="post" class="send">
        <?php
		echo '<input type="hidden" name="groupname" value="'.$groupname.'" />' ;
        php_div_s_1($width-250,250,"新建数据节点","margin-left:15px;");
        ?>
        <div>
            <table width="500" border="0" cellpadding="0" cellspacing="7">
              <tr>
                <td>host name</td>
                <td><input class="input_text" size="50" type="text" name="hostname" /></td>
              </tr>
              <tr>
                <td>service name</td>
                <td><input class="input_text" size="50" type="text" name="servicename" /></td>
              </tr>
              <tr>
                <td>database path</td>
                <td><input class="input_text" size="50" type="text" name="databasepath" /></td>
              </tr>
              <tr>
                <td>config</td>
                <td><input class="input_text" size="50" type="text" name="config" /></td>
              </tr>
              <tr>
                <td></td>
                <td><div align="right"><input name="" type="submit" value=" 创 建 " /></div></td>
              </tr>
            </table>
        </div>
        <?php
        php_div_e_1();
        ?>
        </form>
    </div>
	<div style="margin-left:13px;margin-top:10px;">
		<?php
		if ( $sum > 0 )
		{
			$sum = 0 ;
			$one_or_two = 2 ;
			if ( array_key_exists ( "Group", $arr ) )
			{
				$child_arr = $arr['Group'] ;
				while ( list( $key, $val ) = each ( $child_arr ) )
				{
					if ( $sum == 0 )
					{
						echo '
						<table class="table_form" style="width:'.($width-244).'px;">
						<tr>
							 <td class="table_form_title">数据节点</td>
							 <td class="table_form_title">物理组</td>
							 <td class="table_form_title">服务器名</td>
							 <td class="table_form_title">节点路径</td>
						</tr>' ;
					}
					$arr2 = $child_arr[$key] ;
					$child_child_arr = $arr2['Service'] ;
					if ( list( $key_1, $val_1 ) = each ( $child_child_arr ) )
					{
						$arr3 = $child_child_arr[$key_1] ;
						if ( isStart_2( $db, $groupname, $arr2['HostName'], $arr3['Name'] ) )
						{
							echo '
							<tr>
							  <td class="table_form_context_'.$one_or_two.'"><a href="right_4.php?groupname='.$groupname.'&nodename='.$arr2['HostName'].':'.$arr3 ['Name'].'"><div>'.$arr2['HostName'].':'.$arr3 ['Name'].'</div></a></td>
							  <td  class="table_form_context_'.$one_or_two.'"><a href="right_phy.php?order=selectphy&phyname='.$arr2['HostName'].'" target="right" class="par"><div>'.$arr2['HostName'].'</div></a></td>
							  <td  class="table_form_context_'.$one_or_two.'">'.$arr3['Name'].'</td>
							  <td  class="table_form_context_'.$one_or_two.'">'.$arr2['dbpath'].'</td>
							</tr>
							' ;
						}
						else
						{
							echo '
							<tr class="table_form_context_'.$one_or_two.'">
							  <td>'.$arr2['HostName'].':'.$arr3 ['Name'].'</td>
							  <td>'.$arr2['HostName'].'2</td>
							  <td>'.$arr3['Name'].'</td>
							  <td>'.$arr2['dbpath'].'</td>
							</tr>
							' ;
						}
						if ( $one_or_two == 1 )
						{
							$one_or_two = 2 ;
						}
						else
						{
							$one_or_two = 1 ;
						}
						//echo 'HostName: '.$arr2['HostName'].', Service: '.$arr3 ['Name'] ;
						++$sum ;
					}
				}
			}
			if ( $sum == 0 )
			{
				echo '逻辑组没有数据节点' ;
			}
			else
			{
				echo '</table>' ;
			}
		}
        ?>
    </div>
</div>
</body>
</html>
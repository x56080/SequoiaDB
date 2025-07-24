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

   Source File Name = right_4.php

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
$str = "" ;
$groupname = "" ;
$nodename = "" ;
$hostname = empty($_GET['hostname'])?"":$_GET['hostname'];
$isshow = true ;

if ( !empty ( $_GET['groupname'] ) && !empty ( $_GET['nodename'] ) )
{
	$groupname = $_GET['groupname'] ;
	$nodename = $_GET['nodename'] ;
	$group = $db->selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$node = $group->getNode ( $nodename ) ;
		if ( !empty ( $node ) )
		{
			$db_1 = $node->connect() ;
			$arr_1 = $db->getError() ;
			$db = $db_1 ;
		}
		else
		{
			$isshow = false ;
		}
	}
	else
	{
		$isshow = false ;
	}
}
else if ( !empty ( $_POST['csname'] ) )
{
	$csname = empty( $_POST['csname'] ) ? "" : $_POST['csname'] ;
	$pagesize = empty( $_POST['pagesize'] ) ? 4096 : $_POST['pagesize'] ;
	$str = 'PHP 代码: $cs->selectCS ( "'.$csname.'", "'.$pagesize.'" ) ;' ;
	$db->selectCS ( $csname, $pagesize ) ;
	$arr_2 = $db->getError();
}

if ( !empty ( $_GET['delcsname'] ) )
{
	$delcsname = $_GET['delcsname'] ;
	$cs = $db->selectCS( $delcsname ) ;
	if ( !empty ( $cs ) )
	{
		$cs->drop() ;
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
<!-- jQuery Configuration -->
<!--<script type="text/javascript" src="../js/simpla.jquery.configuration.js"></script>-->
<script type="text/javascript" language="javascript">
function delcs ( delcsname )
{
	if ( confirm ( "确定要删除集合空间吗?" ) )
	{
		window.parent.frames["left"].location.reload() ;
		var temp = 'right_4.php?delcsname=' + delcsname + '<?php echo "&groupname=".$groupname."&nodename=".$nodename."&hostname=".$hostname;?>' ;
		self.location =  temp ;
	}
}
</script>
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
		<a href="right_1.php" class="pathlink"><img src="../images/sever.png" width="15px">&nbsp;服务器：<?php echo $_SESSION['address'] ;?></a> --> 
        <?php
		if ( $hostname != "" )
		{
			echo '<a href="right_phy.php?order=selectphy&phyname='.$hostname.'" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;物理组:'.$hostname.'</a> --> ' ;
		}
		else if ( $groupname != "" )
		{
			echo '<a href="right_group.php?order=selectgroup&groupname='.$groupname.'" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;逻辑组:'.$groupname.'</a> --> ' ;
		}
		if ( $nodename != "" )
		{
			echo '<a href="#" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;单节点'.$nodename.'数据</a>' ;
		}
		else
		{
        	echo '<a href="right_4.php" class="pathlink"><img src="../images/cs.png" width="15px">&nbsp;集合空间列表</a>' ;
		}
		?>
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
			<div class="tbcurrent">
				<img src="../images/cs.png" width="15px" border="0">&nbsp;集合空间
			</div>
		</a>
		<a href="right_5.php">
			<div class="topbutton">
				<img src="../images/editfile.png" width="15px" border="0">&nbsp;SQL
			</div>
		</a>
        <?php
		if ( $groupname != "" && $nodename != "" )
		{
			echo '<a href="right_6.php?model=node&nodename='.$nodename.'">' ;
		}
		else
		{
			echo '<a href="right_6.php?model=all">' ;
		}
		?>
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;数据视图
			</div>
		</a>
        <?php
		if ( $groupname != "" && $nodename != "" )
		{
			echo '<a href="right_7.php?model=node&NodeName='.$nodename.'">' ;
		}
		else
		{
			echo '<a href="right_7.php?model=all">' ;
		}
		?>
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;实时视图
			</div>
		</a>
	</div>
</div>
<?php
if ( $groupname != "" && $nodename != "" && $arr_1 != "" )
{
	php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
	if ( $arr_1['errno'] == 0 )
	{
		echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 当前展示的是单个节点'.$nodename.'的信息' ;
	}
	else
	{
		$isshow = false ;
		echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 连接单节点'.$nodename.'失败 错误码: '.$arr_1['errno'] ;
	}
	php_div_e_3();
}
else if ( $arr_2 != "" )
{
	if ( $arr_2['errno'] == 0 )
	{
		echo '<script type="text/javascript" language="javascript">window.parent.frames["left"].location.reload(); </script>' ;
		//$_SESSION['Group'] = $groupname ;
		//echo '新建collection成功' ;
	}
	else
	{
		$isshow = false ;
		php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
		echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 新建collection失败 错误码: '.$arr_2['errno'] ;
		php_div_e_3();
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
<div style="width:<?php echo $width-230;?>px;margin-top:20px;">
	<div>
        <form action="right_4.php" method="post" class="send">
        <?php
        php_div_s_1($width-250,145,"新建集合空间","margin-top:15px;margin-left:15px;");
        ?>
        <div>
            <table width="500" border="0" cellpadding="0" cellspacing="7">
              <tr>
                <td>集合空间名</td>
                <td><input class="input_text" size="50" type="text" name="csname" /></td>
              </tr>
              <tr>
                <td>分页大小</td>
                <td><!--<input class="input_text" size="50" type="text" name="pagesize" />-->
                	<select name="pagesize" style="width:90px;">
                	  <option value="4096" selected="selected">4096</option>
                	  <option value="8192">8192</option>
                	  <option value="16384">16384</option>
                	  <option value="32768">32768</option>
                	  <option value="65536">65536</option>
                	</select>
                </td>
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
		if ( $isshow )
		{
			$sum = 0 ;
			$cursor = $db->getSnapshot ( SDB_SNAP_COLLECTIONSPACE ) ;
			if ( !empty ( $cursor ) )
			{
				$one_or_two = 2 ;
				while ( $arr = $cursor->getNext() )
				{
					//var_dump ( $arr ) ;
					if ( $sum == 0 )
					{
						echo '
						<table class="table_form" style="width:'.($width-244).'px;">
						<tr>
							<td class="table_form_title">集合空间</td>
							<td class="table_form_title">分页大小</td>
							<td class="table_form_title">集合数</td>
							<td class="table_form_title">操作</td>
						</tr>' ;
					}
					$sum_cl = db_count_cs ( $db, $arr['Name'] ) ;
					$pagesize_temp = "" ;
					if ( array_key_exists( 'PageSize', $arr ) )
						$pagesize_temp = $arr['PageSize'] ;
					echo '
					<tr class="table_form_context_'.$one_or_two.'">
					  <td><a href="right_data.php?order=selectcs&csname='.$arr['Name'].'&groupname='.$groupname.'&nodename='.$nodename.'"><div>'.$arr['Name'].'</div></a></td>
					  <td>'.$pagesize_temp.'</td>
					  <td>'.$sum_cl.'</td>
					  <td><span style="cursor:pointer;" onclick="delcs(\''.$arr['Name'].'\');"><img src="../images/icons/recycle.png" width="15px" border="0" title="删除集合空间"></span></td>
					</tr>
					' ;
					if ( $one_or_two == 1 )
					{
						$one_or_two = 2 ;
					}
					else
					{
						$one_or_two = 1 ;
					}
					++$sum ;
				}
				if ( $sum == 0 )
				{
					echo '没有集合空间' ;
				}
				else
				{
					echo '</table>' ;
				}
			}
		}
        ?>
    </div>
</div>
</body>
</html>
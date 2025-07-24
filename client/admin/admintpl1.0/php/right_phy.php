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

   Source File Name = right_phy.php

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
$phyname = "" ;

if ( !empty( $_GET['order'] ) )
{
	$order = empty( $_GET['order'] ) ? "" : $_GET['order'] ;
	if ( $order == 'selectphy' )
	{
		$phyname = empty( $_GET['phyname'] ) ? "" : $_GET['phyname'] ;
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
		<a href="right_1.php" class="pathlink"><img src="../images/sever.png" width="15px">&nbsp;服务器：<?php echo $_SESSION['address'] ;?></a> --> <a href="#" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;物理组：<?php echo $phyname;?></a>
	</div>
	<div class="clear"></div>

	<div class="topbutton_div">
    	<a href="right_2.php">
			<div class="topbutton">
				<img src="../images/db.png" width="15px" border="0">&nbsp;逻辑组
			</div>
		</a>
        <a href="right_3.php">
			<div class="tbcurrent">
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
		<a href="right_6.php?model=phy&HostName=<?php echo $phyname;?>">
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;数据视图
			</div>
		</a>
        <a href="right_7.php?model=phy&HostName=<?php echo $phyname;?>">
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;实时视图
			</div>
		</a>
	</div>
</div>
<?php
/*
php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
echo '选择物理组成功' ;
php_div_e_3();
*/
?>
<div style="width:<?php echo $width-230;?>px;margin-top:20px;">
	<div style="margin-left:13px;">
		<?php
		if ( $phyname != "" )
		{
			$sum = 0 ;
			$cursor = $db->getList ( SDB_LIST_GROUPS ) ;
			if ( !empty ( $cursor ) )
			{
				echo '
				<table class="table_form" style="width:'.($width-244).'px;">
				<tr>
					  <td class="table_form_title">数据节点</td>
					  <td class="table_form_title">逻辑组</td>
					  <td class="table_form_title">物理组</td>
					  <td class="table_form_title">服务器名</td>
					  <td class="table_form_title">数据节点路径</td>
				</tr>' ;
				$one_or_two = 2 ;
				while ( $arr = $cursor->getNext() )
				{	
					if ( array_key_exists ( "Group", $arr ) )
					{
						$child_arr = $arr['Group'] ;
						while ( list( $key, $val ) = each ( $child_arr ) )
						{
							$arr2 = $child_arr[$key] ;
							$child_child_arr = $arr2['Service'] ;
							if ( list( $key_1, $val_1 ) = each ( $child_child_arr ) )
							{
								$arr3 = $child_child_arr[$key_1] ;
								$is_start = isStart_2 ( $db, $arr['GroupName'], $arr2['HostName'], $arr3['Name'] ) ;
								if ( !strcmp($phyname,$arr2['HostName']) )
								{
									if ( $is_start )
									{
										echo '
										<tr>
										  <td class="table_form_context_'.$one_or_two.'">
										  <a href="right_4.php?groupname='.$arr['GroupName'].'&nodename='.$arr2['HostName'].':'.$arr3 ['Name'].'&hostname='.$arr2['HostName'].'">'.$arr2['HostName'].':'.$arr3 ['Name'].'
										  </a></td>
										  <td class="table_form_context_'.$one_or_two.'"><a href="right_group.php?order=selectgroup&groupname='.$arr['GroupName'].'" target="right" class="par"><div>'.$arr['GroupName'].'</div></a></td>
										  <td class="table_form_context_'.$one_or_two.'">'.$arr2['HostName'].'2</td>
										  <td class="table_form_context_'.$one_or_two.'">'.$arr3['Name'].'</td>
										  <td class="table_form_context_'.$one_or_two.'">'.$arr2['dbpath'].'</td>
										</tr>
										' ;
									}
									else
									{
										echo '
										<tr class="table_form_context_'.$one_or_two.'">
										  <td>'.$arr2['HostName'].':'.$arr3 ['Name'].'</td>
										  <td>'.$arr['GroupName'].'</td>
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
									++$sum ;
								}
							}
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
		}
        ?>
    </div>
</div>
</body>
</html>
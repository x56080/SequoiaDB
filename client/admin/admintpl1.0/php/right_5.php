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

   Source File Name = right_5.php

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
$str = "" ;
$cursor = "" ;

if ( !empty( $_POST['sql'] ) )
{
	$sql = empty( $_POST['sql'] ) ? "" : $_POST['sql'] ;
	$sql = trim($sql) ;
	$temp_arr = str_split ( $sql, 6 ) ;
	if ( !strnatcasecmp ( $temp_arr[0], 'select' ) )
	{
		$str = 'PHP 代码:$db->execSQL ( "'.$sql.'" ) ;' ;
		$cursor = $db->execSQL ( $sql ) ;
		$arr_1 = $db->getError () ;
	}
	else
	{
		$str = 'PHP 代码:$db->execUpdateSQL ( "'.$sql.'" ) ;' ;
		$arr_1 = $db->execUpdateSQL ( $sql ) ;
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
		<a href="right_1.php" class="pathlink"><img src="../images/sever.png" width="15px">&nbsp;服务器：<?php echo $_SESSION['address'] ;?></a> --> <a href="right_5.php" class="pathlink"><img src="../images/editfile.png" width="15px">&nbsp;SQL</a>
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
			<div class="tbcurrent">
				<img src="../images/editfile.png" width="15px" border="0">&nbsp;SQL
			</div>
		</a>
		<a href="right_6.php?model=all">
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;数据视图
			</div>
		</a>
        <a href="right_7.php?model=all">
			<div class="topbutton">
				<img src="../images/searchfile.png" width="15px" border="0">&nbsp;实时视图
			</div>
		</a>
	</div>
</div>
<?php
if ( $arr_1 != "" )
{
	if ( $arr_1['errno'] == 0 )
	{
		//echo '<script type="text/javascript" language="javascript">window.parent.frames["left"].location.reload(); <///script>' ;
		//$_SESSION['Group'] = $groupname ;
		// '执行SQL命令成功' ;
	}
	else
	{
		php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
		echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 执行SQL命令失败 错误码: '.$arr_1['errno'] ;
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
<div style="width:<?php echo $width-230;?>px;margin-top:10px;">
	<div>
        <form action="right_5.php" method="post" class="send">
        <?php
        php_div_s_1($width-250,175,"在数据库运行 SQL 查询","margin-top:15px;margin-left:15px;");
        ?>
            <div style="padding:0 0 10px 10px;">
                <textarea name="sql" rows="5" cols="100"></textarea>
            </div>
            <div style="width:650px;" align="right">
            	<input type="submit" value=" 执 行 "/>
            </div>
        <?php
        php_div_e_1();
        ?>
        </form>
    </div>
	<div style="padding-left:13px;margin-top:10px;">
    <?php
		if ( $cursor != "" )
		{
			$db->install ( "{install:false}" ) ;
			$sum = 0 ;
			$one_or_two = 2 ;
			while ( $str_return = $cursor->getNext() )
			{
				if ( $sum == 0 )
				{
					echo '
					<div class="div_table_1" style="width:'.($width-240).'px;">
					<div class="div_table_context_1" style="width:'.($width-260).'px;">        
						sql的执行结果
					</div>' ;
				}
				echo'
				<div class="div_table_context_'.$one_or_two.'" style="width:'.($width-260).'px;">        
					'.htmlspecialchars($str_return).'
				</div>' ;
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
			if ( $sum > 0 )
			{
				echo '</div>' ;
			}
			else
			{
				echo '没有数据' ;
			}
		}
        ?>
    </div>
</div>
</body>
</html>
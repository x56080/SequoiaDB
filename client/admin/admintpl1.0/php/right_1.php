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

   Source File Name = right_1.php

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

$isGroup = false ;
$cursor = $db->getList ( SDB_LIST_GROUPS ) ;
$arr = $db->getError () ;
if ( $arr['errno'] == 0 )
{
	$isGroup = true ;
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
	<div class="clear"></div>
<!-- /topNavWrapper -->

<!-- topbutton -->
<div class="topb">
	<div class="path">
		<a href="#" class="pathlink"><img src="../images/sever.png" width="15px">&nbsp;服务器：<?php echo $_SESSION['address'] ;?></a>
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
        <?php
		if ( $isGroup )
		{
			echo '<a href="right_6.php?model=all">';
		}
		else
		{
			echo '<a href="right_6.php?model=node">';
		}
		?>
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

<div style="width:<?php echo $width-300;?>px;">

<div style="float:left;width:<?php echo $width-780;?>px;padding-left:20px;">
	<div>
        <?php
        php_div_s_1($width-800,100,"操作");
        ?>
        <div style="padding-left:10px;padding-top:5px;padding-bottom:15px">修改密码</div>
        <div style="padding-left:10px;">退出</div>
        <?php
        php_div_e_1();
        ?>
    </div>
    <div style="padding-top:15px;">
        <form action="right_group.php" method="post">
        <?php
        php_div_s_1($width-800,100,"新建逻辑组");
        ?>
        <div style="padding:12px;">逻辑组名 <input class="input_text" type="text" name="create_group_name" /> <input name="" type="submit" value=" 创 建 " /></div>
        <?php
        php_div_e_1();
        ?>
        </form>
    </div>
    <div style="padding-top:15px;">
        <?php
        php_div_s_1($width-800,240,"集群状态","","padding:0px;height:208px;width:".($width-800)."px;");
        ?>
        <div>
            <div style="width:<?php echo ($width-800)/2;?>px;height:178px;padding-top:30px;padding-left:30px;float:left;border-right-style:solid;border-right-width:1px;border-right-color:#CED5DA;">
                <div style="height:208px; float:left;"><img src="../images/11.png" /></div>
                <div style="height:168px; float:left; padding-left:30px; padding-top:40px;">活动的逻辑组: </div>
            </div>
            <div style="height:208px; float:left;width:<?php echo ($width-800)/2-31;?>px;">
                <div style="height:74px;padding-top:30px;padding-left:20px;border-bottom-style:solid;border-bottom-color:#CED5DA;border-bottom-width:1px;">
                    <div style="height:94px; float:left;"><img src="../images/12.png" /></div>
                    <div style="height:94px; float:left; padding-left:10px; padding-top:10px;">活动的物理组: </div>
                </div>
                <div style="height:73px;padding-top:30px;padding-left:20px;border-bottom-style:solid;border-bottom-color:#CED5DA;border-bottom-width:1px;">
                    <div style="height:93px; float:left;"><img src="../images/12.png" /></div>
                    <div style="height:93px; float:left; padding-left:10px; padding-top:10px;">停止的逻辑组: </div>
                </div>
            </div>
        </div>
        <?php
        php_div_e_1();
        ?>
    </div>
</div>

<div style="float:left;width:450px;">
	<div>
		<?php
        php_div_s_1(440,200,"SequoiaDB");
        ?>
        <table width="100%" border="0" cellspacing="14" cellpadding="2">
          <tr>
            <td>服务器:</td>
            <td><?php echo $_SESSION['address'] ;?> TCP/IP</td>
          </tr>
          <tr>
            <td>服务器版本:</td>
            <td>1.0.0</td>
          </tr>
          <tr>
            <td>协议版本:</td>
            <td>10</td>
          </tr>
          <tr>
            <td>服务器操作系统:</td>
            <td><?php echo PHP_OS;?></td>
          </tr>
          <tr>
            <td>用户:</td>
            <td><?php echo $_SESSION['user']; ?></td>
          </tr>
        </table>
        <?php
        php_div_e_1();
        ?>
    </div>
    <div style="padding-top:15px;">
		<?php
        php_div_s_1(440,120,"网站服务器");
        ?>
        <table width="100%" border="0" cellspacing="12" cellpadding="2">
          <tr>
            <td></td>
          </tr>
          <tr>
            <td>PHP 扩展: SequoiaDB <?php echo phpversion( 'SequoiaDB' );?></td>
          </tr>
        </table>
        <?php
        php_div_e_1();
        ?>
    </div>
    <div style="padding-top:15px;">
		<?php
        php_div_s_1(440,120,"phpSequoiaDB");
        ?>
        <table width="100%" border="0" cellspacing="8" cellpadding="2">
          <tr>
            <td>版本信息: 2.0.2</td>
          </tr>
          <tr>
            <td>文档(外链)</td>
          </tr>
          <tr>
            <td>官网主页(外链)</td>
          </tr>
        </table>
        <?php
        php_div_e_1();
        ?>
    </div>
</div>
</div>
</body>
</html>
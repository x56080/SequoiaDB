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

   Source File Name = right_update.php

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
$cl = "" ;
$str = "" ;
$order = empty ( $_GET['order'] ) ? "" : $_GET['order'] ;
$groupname = empty ( $_GET['groupname'] ) ? "" : $_GET['groupname'] ;
$nodename = empty ( $_GET['nodename'] ) ? "" : $_GET['nodename'] ;
$csname = empty ( $_GET['csname'] ) ? "" : $_GET['csname'] ;
$clname = empty ( $_GET['clname'] ) ? "" : $_GET['clname'] ;
$surl = empty ( $_GET['surl'] ) ? $_SERVER['HTTP_REFERER'] : $_GET['surl'] ;
if ( $order == "update" )
{
	$rule = empty ( $_GET['rule'] ) ? "" : $_GET['rule'] ;
	$condition = empty ( $_GET['condition'] ) ? "" : $_GET['condition'] ;
	$hint = empty ( $_GET['hint'] ) ? "" : $_GET['hint'] ;
	if ( $groupname != "" && $nodename != "" )
	{
		if ( $csname != "" && $clname != "" )
		{
	
			$group = $db->selectGroup ( $groupname ) ;
			$arr_1 = $db->getError() ;
			if ( $arr_1['errno'] == 0 )
			{
				$node = $group->getNode ( $nodename ) ;
				$arr_1 = $db->getError() ;
				if ( $arr_1['errno'] == 0 )
				{
					$db = $node->connect() ;
					$arr_1 = $db->getError() ;
					if ( $arr_1['errno'] == 0 )
					{
						$cs = $db->selectCS ( $csname ) ;
						$arr_1 = $db->getError() ;
						if ( $arr_1['errno'] == 0 )
						{
							$cl = $cs->selectCollection ( $clname ) ;
							$arr_1 = $db->getError() ;
							if ( $arr_1['errno'] == 0 )
							{
								$cl->update ( $rule, $condition, $hint ) ;
								$arr_1 = $db->getError() ;
							}
						}
					}
				}
			}
		}
	}
	else if ( $csname != "" && $clname != "" )
	{
		$cs = $db->selectCS ( $csname ) ;
		$arr_1 = $db->getError() ;
		if ( $arr_1['errno'] == 0 )
		{
			$cl = $cs->selectCollection ( $clname ) ;
			$arr_1 = $db->getError() ;
			if ( $arr_1['errno'] == 0 )
			{
				$cl->update ( $rule, $condition, $hint ) ;
				$arr_1 = $db->getError() ;
			}
		}
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
		<a href="right_1.php" class="pathlink"><img src="../images/sever.png" width="15px">&nbsp;服务器：<?php echo $_SESSION['address'] ;?></a> --> 
        <?php
		if ( $groupname != "" && $nodename != "" )
		{
			echo '<a href="right_4.php?groupname='.$groupname.'&nodename='.$nodename.'" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;单节点'.$nodename.'数据</a>' ;
		}
		else
		{
        	echo '<a href="right_4.php" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;集合空间列表</a>' ;
		}
		?>
         --> <a href="right_data.php?order=selectcs&csname=<?php echo $csname;?>&groupname=<?php echo $groupname;?>&nodename=<?php echo $nodename;?>" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;集合空间：<?php echo $csname;?></a> 
         --> <a href="#" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;修改数据</a>
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
		if ( $nodename != "" )
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
		if ( $nodename != "" )
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
if ( $arr_1  != "" )
{
	if ( $arr_1['errno'] == 0 )
	{
		
		echo '<script type="text/javascript" language="javascript">window.parent.frames["right"].location.href="'.$_GET['rurl'].'"; </script>' ;
	}
	else
	{
		php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
		echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 数据更新失败 错误码: '.$arr_1['errno'] ;
		php_div_e_3();
	}
}
if ( $str != "" )
{
	php_div_s_3($width-250,"background-color:#F8F8F8;margin-left:15px;");
	echo $str ;
	php_div_e_3();
}
?>
<div style="width:<?php echo $width-206;?>px;margin-top:20px;">
    <div>
        <form action="right_update.php" method="get" class="send">
        <input type="hidden" name="rurl" value="<?php echo $surl;?>" />
        <input type="hidden" name="groupname" value="<?php echo $groupname;?>" />
        <input type="hidden" name="nodename" value="<?php echo $nodename;?>" />
        <input type="hidden" name="csname" value="<?php echo $csname;?>" />
        <input type="hidden" name="clname" value="<?php echo $clname;?>" />
        <input type="hidden" name="order" value="update" />
        <?php
		$temp_str = "在" ;
		if ( $groupname != "" && $nodename != "" )
		{
			$temp_str = $temp_str."分区组 <b>".$groupname."</b> 的数据节点 <b>".$nodename."</b> 的" ;
		}
		if ( $csname != "" && $clname != "" )
		{
			$temp_str = $temp_str."<b>".$csname.".".$clname."</b> 中更新数据" ;
		}
        php_div_s_1($width-250,180,$temp_str,"margin-top:15px;margin-left:15px;");
        ?>
        <div style="padding:12px;">
            <table width="500" border="0" cellpadding="0" cellspacing="7">
           	   <tr>
                <td>rule  </td>
                <td><input name="rule" type="text" /></td>
              </tr>
              <tr>
                <td>condition  </td>
                <td><input name="condition" type="text" /></td>
              </tr>
              <tr>
                <td>hint  </td>
                <td><input name="hint" type="text" /></td>
              </tr>
              <tr>
                <td></td>
                <td><div align="right"><input name="" type="submit" value=" 更 新 " /></div></td>
              </tr>
            </table>
        </div>
        <?php
        php_div_e_1();
        ?>
        </form>
    </div>
</div>
</body>
</html>
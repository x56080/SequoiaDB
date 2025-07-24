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

   Source File Name = right_data.php

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
$csname = "" ;
$str = "" ;
$arr_1 = "" ;
$arr_2 = "" ;
$cs = "" ;
$groupname = "" ;
$nodename = "" ;
$isshow = true ;

if ( !empty( $_POST['clname'] ) )
{
	$csname = $_SESSION['csname'] ;
	$clname = empty( $_POST['clname'] ) ? "" : $_POST['clname'] ;
	$shardingkey = empty( $_POST['shardingkey'] ) ? "" : $_POST['shardingkey'] ;
	$str = 'PHP 代码: $cs->selectCollection ( "'.$clname.'", "'.$shardingkey.'" ) ;' ;
	$cs = $db->selectCS ( $csname ) ;
	$arr_1 = $db->getError();
	if ( $arr_1['errno'] == 0 )
	{
		$cl = $cs->selectCollection ( $clname, $shardingkey ) ;
		$arr_1 = $db->getError();
	}
}
else if ( !empty( $_GET['order'] ) )
{
	$order = empty( $_GET['order'] ) ? "" : $_GET['order'] ;
	if ( $order == 'selectcs' )
	{
		$csname = empty( $_GET['csname'] ) ? "" : $_GET['csname'] ;
		$groupname = empty( $_GET['groupname'] ) ? "" : $_GET['groupname'] ;
		$nodename = empty( $_GET['nodename'] ) ? "" : $_GET['nodename'] ;
		$_SESSION['csname']  = $csname ;
		if ( $groupname != "" && $nodename != "" )
		{
			$group = $db->selectGroup ( $groupname ) ;
			$node = $group->getNode ( $nodename ) ;
			$db = $node->connect() ;
		}
		$cs = $db->selectCS ( $csname ) ;
		$arr_2 = $db->getError();
		$str = 'PHP 代码: $db->selectCS ( "'.$csname.'" ) ;' ;
	}
}

if ( !empty ( $_GET['delcsname'] ) && !empty($_GET['delclname']) )
{
	$delcsname = $_GET['delcsname'] ;
	$delclname = $_GET['delclname'] ;
	$cs = $db->selectCS( $delcsname ) ;
	if ( !empty ( $cs ) )
	{
		$cl = $cs->selectCollection ( $delclname ) ;
		if ( !empty($cl) )
		{
			$cl->drop() ;
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
function delcollection(delcsname,collname)
{
	if ( confirm ( "确定要删除集合吗?" ) )
	{
		var temp = 'right_data.php?delcsname=' + delcsname + '&delclname=' + collname + '<?php echo "&groupname=".$groupname."&nodename=".$nodename."&csname=".$csname."&order=selectcs";?>' ;
		self.location = temp ;
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
		if ( $groupname != "" && $nodename != "" )
		{
			echo '<a href="right_4.php?groupname='.$groupname.'&nodename='.$nodename.'" class="pathlink"><img src="../images/db.png" width="15px">&nbsp;单节点'.$nodename.'数据</a>' ;
		}
		else
		{
        	echo '<a href="right_4.php" class="pathlink"><img src="../images/cs.png" width="15px">&nbsp;集合空间列表</a>' ;
		}
		?>
         --> <a href="#" class="pathlink"><img src="../images/cl.png" width="15px">&nbsp;集合空间：<?php echo $csname;?></a>
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
if ( $arr_1 != "" || $arr_2 != "" )
{
	if ( $arr_1  != "" )
	{
		if ( $arr_1['errno'] == 0 )
		{
			//echo '<script type="text/javascript" language="javascript">window.parent.frames["left"].location.reload(); <///script>' ;
			//$_SESSION['Group'] = $groupname ;
			//echo '新建集合成功' ;
		}
		else
		{
			$isshow = false ;
			php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
			echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 新建集合失败 错误码: '.$arr_1['errno'] ;
			php_div_e_3();
		}
	}
	else if ( $arr_2 != "" )
	{
		if ( $arr_2['errno'] == 0 )
		{
			//echo '选择集合空间成功<br/>' ;
			if ( $groupname != "" && $nodename != "" && $arr_2['errno'] == 0 )
			{
				echo '<img src="../images/icons/exclamation.png" width="15px" border="0">当前展示的是单个节点'.$nodename.'的信息' ;
			}
		}
		else
		{
			$isshow = false ;
			php_div_s_3($width-250,"margin-left:15px;margin-top:15px;");
			echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 选择集合空间失败 错误码: '.$arr_2['errno'] ;
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
<div style="width:<?php echo $width-230;?>px;margin-top:20px;">
	<div>
        <form action="right_data.php" method="post" class="send">
        <?php
        php_div_s_1($width-250,160,"新建集合","margin-top:15px;margin-left:15px;");
        ?>
        <div>
            <table width="500" border="0" cellpadding="0" cellspacing="7">
              <tr>
                <td>集合名</td>
                <td><input class="input_text" size="50" type="text" name="clname" /></td>
              </tr>
              <tr>
                <td>options</td>
                <td><input class="input_text" size="50" type="text" name="shardingkey" /></td>
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
		ini_set ( "register_globals","on" ) ;
		if ( isset( $_SERVER['QUERY_STRING'] ) )
		{
		   $_SESSION['surl'] = $_SERVER['PHP_SELF'].'?'.$_SERVER['QUERY_STRING'] ;
		}
		else
		{
		   $_SESSION['surl'] = $_SERVER['PHP_SELF'] ;
		}
		if ( $isshow && $csname != "" )
		{
			$sum = 0 ;
			$cursor = $db->listCollections(); // ( SDB_SNAP_COLLECTION ) ;
			if ( !empty ( $cursor ) )
			{
				$one_or_two = 2 ;
				while ( $arr = $cursor->getNext() )
				{
					$str_temp = $arr['Name'] ;
					$arr_return =  explode ( ".", $str_temp ) ;
					if ( $arr_return[0] == $csname )
					{
						if ( $sum == 0 )
						{
							echo '
							<table class="table_form" style="width:'.($width-244).'px;">
							<tr>
								  <td class="table_form_title">集合名</td>
								  <td class="table_form_title">所属的逻辑组</td>
								  <td class="table_form_title">增,删,改,切分操作</td>
								  <td class="table_form_title">表操作</td>
							</tr>' ;
						}
						$fromGroup = array() ;
						if ( $cs != "" )
						{
							$cl = $cs->selectCollection ( $arr_return[1] ) ;
							if ( !empty ( $cl ) )
							{
								$fromGroup = $db->getSnapshot(SDB_SNAP_COLLECTION) ;
							}
						}
						echo '
						<tr>
						  <td class="table_form_context_'.$one_or_two.'"><a href="right_notes.php?order=find&csname='.$arr_return[0].'&clname='.$arr_return[1].'&groupname='.$groupname.'&nodename='.$nodename.'"><div>'.$arr_return[1].'</div></a></td>
						  <td class="table_form_context_'.$one_or_two.'">';
						  if ( !empty ( $fromGroup ) )
						  {
							//$db->install ( array ( "install" => false ) ) ;
							while ( $fromGroupArr = $fromGroup->getNext() )
							{
								
								//echo  $fromGroupArr ;
								if ( $fromGroupArr["Name"] == $str_temp )
								{
									if ( array_key_exists ( "CataInfo", $fromGroupArr ) )
									{
						  				while ( list( $key, $val ) = each ( $fromGroupArr["CataInfo"] ) )
						  				{
						  					echo $val["GroupName"]." " ;
						  				}
									}
								}
							}
							//$db->install ( array ( "install" => true ) ) ;
						  }
						  echo '</td>
						  <td width="200" class="table_form_context_'.$one_or_two.'">
						  	<a href="right_insert.php?csname='.$arr_return[0].'&clname='.$arr_return[1].'&groupname='.$groupname.'&nodename='.$nodename.'">
								<img title="插入" src="../images/icons/pencil.png"/>
							</a>&nbsp;
							<a href="right_delete.php?csname='.$arr_return[0].'&clname='.$arr_return[1].'&groupname='.$groupname.'&nodename='.$nodename.'">
								<img title="删除" src="../images/icons/cross_circle.png"/>
							</a>&nbsp;
							<a href="right_update.php?csname='.$arr_return[0].'&clname='.$arr_return[1].'&groupname='.$groupname.'&nodename='.$nodename.'"> 
								<img title="修改" src="../images/icons/hammer_screwdriver.png"/>
							</a>&nbsp;
							<a href="right_split.php?csname='.$arr_return[0].'&clname='.$arr_return[1].'&groupname='.$groupname.'&nodename='.$nodename.'">
								<img title="切分" src="../images/icons/split.png"/>
							</a>
						  </td>
						  <td width="100" class="table_form_context_'.$one_or_two.'"><span style="cursor:pointer;" onclick="delcollection(\''.$arr_return[0].'\',\''.$arr_return[1].'\')"><img src="../images/icons/recycle.png" border="0" title="删除集合"></span></td>
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
				}
				if ( $sum == 0 )
				{
					echo '集合空间没有集合' ;
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
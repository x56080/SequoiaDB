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

   Source File Name = right_notes_tm.php

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
$clname = "" ;
$str = "" ;
$start_note = 0 ;
$num_note = 0 ;
$sum_note = 0 ;

if ( !empty( $_GET['order'] ) )
{
	$order = empty( $_GET['order'] ) ? "" : $_GET['order'] ;
	if ( $order == 'find' )
	{
		$csname = empty( $_GET['csname'] ) ? "" : $_GET['csname'] ;
		$clname = empty( $_GET['clname'] ) ? "" : $_GET['clname'] ;
		$groupname = empty( $_GET['groupname'] ) ? "" : $_GET['groupname'] ;
		$nodename = empty( $_GET['nodename'] ) ? "" : $_GET['nodename'] ;
		$start_note = empty( $_GET['start_note'] ) ? 0 : $_GET['start_note'] ;
		$num_note = empty( $_GET['num_note'] ) ? 30 : $_GET['num_note'] ;
		$hand = empty( $_GET['hand'] ) ? -1 : $_GET['hand'] ;
		if ( $groupname != "" && $nodename != "" )
		{
			$group = $db->selectGroup ( $groupname ) ;
			$node = $group->getNode ( $nodename ) ;
			$db = $node->connect() ;
		}
		if ( $csname != "" && $clname != "" )
		{
			$cs = $db->selectCS ( $csname ) ;
			if ( !empty ( $cs ) )
			{
				$cl = $cs->selectCollection ( $clname ) ;
				if ( !empty ( $cl ) )
				{
					$sum_note = $cl->count () ;
				}
			}
		}
		if ( $hand == '    <<    ' )
		{
			$start_note = 0 ;
			if ( $num_note <= 0 )
			{
				$num_note = 1 ;
			}
		}
		else if ( $hand == '   <   ' )
		{
			$start_note = $start_note - $num_note ;	
			if ( $start_note < 0 )
			{
				$start_note = 0 ;
			}
		}
		else if ( $hand == '    >>    ' )
		{
			//$num_note = 30 ;
			$start_note = $sum_note - $num_note ;
			if ( $start_note < 0 )
			{
				$start_note = 0 ;
			}
		}
		else if ( $hand == '   >   ' )
		{
			$start_note = $start_note + $num_note ;
			if ( $start_note >= $sum_note )
			{
				$start_note = $sum_note - $num_note ;
				if ( $start_note < 0 )
				{
					$start_note = 0 ;
				}
			}
		}
		$str = 'PHP 代码: $cursor = $cl->find ( NULL, NULL, NULL, NULL, '.$start_note.', '.($start_note + $num_note).' ) ;' ;
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
<link rel="stylesheet" href="../css/table_json.css" type="text/css"/>

<!--                       Javascripts                       -->
<!-- jQuery -->
<script src="../js/jquery1.3.2.js" type="text/javascript"></script>
<!--<script type="text/javascript" src="../js/jquery-1.3.2.min.js"></script>-->
<script type="text/javascript" language="javascript" src="../js/common.js"></script>
<script type="text/javascript" language="javascript">
$(document).ready(function(){
  $("form").submit(function(e){
    //e.preventDefault();
    //alert("Submit prevented");
	e.fromElement.submit();
  });
});
function show_hide( num )
{
	$("#child_"+num).toggle(200);
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
	    <li style="list-style-type:none;">
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
         --> 
        <a href="right_data.php?order=selectcs&csname=<?php echo $csname;?>&groupname=<?php echo $groupname;?>&nodename=<?php echo $nodename;?>" class="pathlink"><img src="../images/cl.png" width="15px">&nbsp;集合空间：<?php echo $csname;?></a> 
         --> <a href="#" class="pathlink"><img src="../images/select.png" width="15px">&nbsp;集合：<?php echo $clname;?></a>
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
//if ( $str != "" )
//{
	if ( $groupname != "" && $nodename != "" )
	{
		php_div_s_3($width-250,"background-color:#F8F8F8;margin-left:15px;margin-top:15px;");
		echo '<img src="../images/icons/exclamation.png" width="15px" border="0"> 当前展示的是单个节点'.$nodename.'的信息<br/>' ;
		php_div_e_3();
	}
	//echo $str ;	
//}
$_SESSION['surl'] = $_SERVER['PHP_SELF'].'?'.$_SERVER['QUERY_STRING'] ;
?>
<div style="width:<?php echo $width-206;?>px;margin-top:30px;margin-bottom:60px;">
	<div style="margin-left:15px;">
    	<a href="<?php echo 'right_notes.php?order=find&csname='.$csname.'&clname='.$clname.'&groupname='.$groupname.'&nodename='.$nodename;?>" class="button2">切换成字符串</a>&nbsp;&nbsp;
        <a href="<?php echo 'right_insert.php?csname='.$csname.'&clname='.$clname.'&groupname='.$groupname.'&nodename='.$nodename;?>" class="button2">插入数据</a>&nbsp;&nbsp;
        <a href="<?php echo 'right_delete.php?csname='.$csname.'&clname='.$clname.'&groupname='.$groupname.'&nodename='.$nodename;?>" class="button2">删除数据</a>&nbsp;&nbsp;
        <a href="<?php echo 'right_update.php?csname='.$csname.'&clname='.$clname.'&groupname='.$groupname.'&nodename='.$nodename;?>" class="button2">修改数据</a>
    </div>
	<div style="margin-left:15px;margin-top:30px;">
        <div style="padding-bottom:15px;">
        <form action="right_notes_tm.php" method="get" class="send" id="send_1">
        	<input type="hidden" name="groupname" value="<?php echo $groupname;?>" />
        	<input type="hidden" name="nodename" value="<?php echo $nodename;?>" />
            <input type="hidden" name="sum_note" value="<?php echo $sum_note;?>" />
            <input type="hidden" name="csname" value="<?php echo $csname;?>" />
            <input type="hidden" name="clname" value="<?php echo $clname;?>" />
            <input type="hidden" name="order" value="find" />
            <input type="submit" name="hand" value="    <<    "/> <input type="submit" name="hand" value="   <   " /> <input type="submit" value="  显示:  " /> <input size="5" name="num_note" value="<?php echo $num_note;?>" type="text" /> 行 开始行数: <input size="5" name="start_note" value="<?php echo $start_note;?>" type="text" /> <input type="submit" name="hand" value="   >   " /> <input type="submit" name="hand" value="    >>    " />
            <!--页码: 
            <select name="" onchange="send_1.submit()">
              <option value="1" selected="selected">1</option>
              <option value="2">2</option>
            </select>-->
        </form>
        </div>
        <?php
		$sum_notes = 0 ;
		$temp_width = $width - 260 ;
		if ( $sum_note > 0 )
		{
			//$db->install ( "{install:false}" ) ;
			$cursor = $cl->find ( NULL, NULL, NULL, NULL, $start_note, $num_note ) ;
			if ( !empty ( $cursor ) )
			{
				$sum = 0 ;
				while ( $str_return = $cursor->getNext() )
				{
					if ( $sum == 0 )
					{
						echo '
						<div style="width:'.($temp_width-12).'px;border:1px solid #999;padding:6px;height:19px;background:#E8E8E8"><b>集合 '.$clname.' 的记录</b></div>
						<div style="width:'.($temp_width).'px;border:1px solid #999;height:32px;border-top:0px;">
							<div style="width:'.($temp_width/3-10).'px;float:left;height:20px;padding:6px;">Key</div>
							<div style="width:'.($temp_width/2-10).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">Value</div>
							<div style="width:'.($temp_width/6-24).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">Type</div>
						</div>' ;
					}
					$db->install ( "{install:false}" ) ;
					$str_return_s = $cursor->current() ;
					$db->install ( "{install:true}" ) ;
					echo'
						<div class="div_table_1" style="width:'.($width-272).'px;border:1px solid #999;height:20px;border-top:0px;padding:6px;cursor:pointer;background-color:#EFEFEF;" onclick="show_hide(\''.$sum.'\')">
						('.($sum+$start_note).')'.substr($str_return_s,0,55).'...
						</div>';
					$arr_line = array () ;
					array_push ( $arr_line, true ) ;
					echo '<div id="child_'.$sum.'" style="display:none">' ;
					echo json2table ( $str_return, $temp_width, 0, 0, $arr_line, "" ) ;
					echo '</div>' ;
					++$sum ;
					++$sum_notes ;
				}
			}
		}
		else
		{
			echo 'Collection没有记录 ' ;
		}
		if ( $sum_notes > 10 )
		{
        ?>
        <div style="padding-bottom:15px;margin-top:15px;">
        <form action="right_notes_tm.php" method="get" class="send" id="send_1">
        	<input type="hidden" name="groupname" value="<?php echo $groupname;?>" />
        	<input type="hidden" name="nodename" value="<?php echo $nodename;?>" />
            <input type="hidden" name="sum_note" value="<?php echo $sum_note;?>" />
            <input type="hidden" name="csname" value="<?php echo $csname;?>" />
            <input type="hidden" name="clname" value="<?php echo $clname;?>" />
            <input type="hidden" name="order" value="find" />
            <input type="submit" name="hand" value="    <<    "/> <input type="submit" name="hand" value="   <   " /> <input type="submit" value="  显示:  " /> <input size="5" name="num_note" value="<?php echo $num_note;?>" type="text" /> 行 开始行数: <input size="5" name="start_note" value="<?php echo $start_note;?>" type="text" /> <input type="submit" name="hand" value="   >   " /> <input type="submit" name="hand" value="    >>    " />
            <!--页码: 
            <select name="" onchange="send_1.submit()">
              <option value="1" selected="selected">1</option>
              <option value="2">2</option>
            </select>-->
        </form>
        </div>
        <?php
		}
		?>
    </div>
</div>
</body>
</html>
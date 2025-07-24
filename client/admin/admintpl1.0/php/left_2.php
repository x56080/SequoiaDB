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

   Source File Name = left_2.php

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

<link rel='stylesheet' href='../css/left_style.css' type='text/css'/>

</head>
<body>
<div class="aleft">
<!-- abody.left -->
<div style="cursor:pointer;" onclick="javascript:window.parent.location.reload();">
<img src="../images/orange_logo.png" width="160px">
</div>
<div class="clear"></div>
<!-- left.topbutton -->
<div class="left_topbutton_div">
	<a href="left_1.php">
		<div class="left_topbutton">
			逻辑
		</div>
	</a>
	<a href="left_2.php">
		<div class="ltbcurrent">
			物理
		</div>
	</a>
	<a href="left_3.php">
		<div class="left_topbutton">
			集合空间
		</div>
	</a>
</div>
<div class="clear"></div>
<!-- /left.topbutton -->
<div class="menu_search">
    <img src="../images/search.png" width="16px"> <input id="search" type="text" name="s" value="" onchange="hiddenli();" size="16"/>
	<script type="text/javascript">
	function hiddenli ()
	{
		var titlestr = document.getElementById("search").value ;
		var liList = document.getElementById("list").childNodes;
		for(var i = 0; i < liList.length; i ++)
		{
			if( liList[i].innerHTML != undefined )//&& liList[i].innerHTML == titlestr )
			{
				var temp_2 = liList[i].innerHTML.lastIndexOf ( '</a>' ) ;
				var temp_1 = liList[i].innerHTML.lastIndexOf ( '>', temp_2 ) ;
				var temp = liList[i].innerHTML.substring ( temp_1 + 1, temp_2 ) ;
				//alert ( temp ) ;
				if ( titlestr == "" )
				{
					liList[i].style.display = "block" ;
				}
				else if ( temp.indexOf ( titlestr ) < 0 )
				{
					liList[i].style.display = "none" ;
				}
				else if ( temp.indexOf ( titlestr ) >= 0 )
				{
					liList[i].style.display = "block" ;
				}
			}
		}
	}
    </script>
</div>
<div class="clear"></div>
<!-- tree menu -->
        <div class="emenu_nav">
            <ul id="list">
                <?php
				if ( empty($_SESSION['address']) )
				{
					echo '<script type="text/javascript" language="javascript">window.parent.location.href="../login.php"; </script>' ;
					exit() ;
				}
				$db = new SequoiaDB() ;
				$db->connect ( $_SESSION['address'], $_SESSION['user'], $_SESSION['password'] ) ;
				$arr = $db->getError () ;
				if ( $arr['errno'] )
				{
					echo '<script type="text/javascript" language="javascript">window.parent.location.href="../login.php"; </script>' ;
					exit() ;
				}
				$cursor = $db->getList ( SDB_LIST_GROUPS ) ;
				$arr = $db->getError () ;
				if ( $arr['errno'] == 0 )
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
							$cursor_1 = $db->getSnapshot ( SDB_SNAP_DATABASE, $str ) ;
							if ( !empty($cursor_1) )
							{
								$arr_data = $cursor_1->getNext() ;
								if ( !in_array ( $arr_data['HostName'], $phy_arr ) )
								{
									array_push ( $phy_arr, $arr_data['HostName'] ) ;
				?>
                <li class="level1">
					<img src="../images/severone.png" width="15px"><a href="right_phy.php?order=selectphy&phyname=<?php echo $arr_data['HostName'];?>" target="right" class="par"><?php echo $arr_data['HostName'];?></a>
				</li>
                <?php
								}
							}
						}
					}
				}
				?>
            </ul>
        </div>
<!-- /treemenu -->
</div>
<!-- /abody.left -->
</body>
</html>
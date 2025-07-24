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

   Source File Name = php_lib.php

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
function php_div_s_1( $width, $height, $title, $additive="", $additive2="" )
{
echo '<div class="div_frame_1" style="width:'.$width.'px;height:'.$height.'px;'.$additive.'"><div class="div_frame_title_1" style="width:'.($width-20).'px;">'.$title.'</div><div class="div_frame_context_1" style="width:'.($width-20).'px;height:'.($height-52).'px;'.$additive2.'">' ;
}

function php_div_e_1()
{
echo '</div></div>';
}

function php_div_s_2( $width, $height, $additive="" )
{
echo '<div class="div_frame_1" style="width:'.$width.'px;height:'.$height.'px;'.$additive.'"><div class="div_frame_title_1" style="width:'.($width-20).'px;">';
}

function php_div_c_2( $width, $height )
{
echo '</div><div class="div_frame_context_2" style="width:'.($width-20).'px;height:'.($height-84).'px;">' ;
}

function php_div_b_2( $width )
{
echo '</div><div class="div_frame_bottom_2" style="width:'.($width-20).'px;">' ;
}

function php_div_e_2()
{
echo '</div></div>';
}

function php_div_s_3( $width, $additive="" )
{
echo '<div class="div_frame_context_3" style="width:'.($width-20).'px;'.$additive.'"><b>';
}

function php_div_e_3()
{
echo '</b></div></div>';
}

function getNumber( $str )
{
	if ( is_object ( $str ) )
	{
		return $str->__toString() ;
	}
	else
	{
		return $str ;
	}
}

function isStart( $db, $arr )
{
	if ( !array_key_exists ( "Group", $arr ) )
	{
		return false;
	}
	$child_arr = $arr['Group'] ;
	$sum = 0 ;
	while ( list( $key, $val ) = each ( $child_arr ) )
	{
		//echo json_encode($child_arr) ;
		$arr2 = $child_arr[$key] ;
		/*$str = '{"GroupID":'.$arr['GroupID'].',"NodeID":'.$arr2['NodeID'].'}' ;
		$cursor1 = $db->getSnapshot ( SDB_SNAP_DATABASE, $str ) ;
		if ( empty($cursor1) )
		{
			echo $str ;
			return false ;
		}
		$arr_1 = $cursor1->getNext() ;*/
		$group = $db->selectGroup ( $arr['GroupName'] ) ;
		$err = $db->getError () ;
		if ( $err['errno'] != 0 )
		{
			return false ;
		}
		//$node = $group->getNode ( $arr_1['HostName'].':'.$arr_1['ServiceName'] ) ;
		$node = $group->getMaster() ;
		$err = $db->getError () ;
		if ( $err['errno'] != 0 )
		{
			return false ;
		}
		$db_2 = $node->connect() ;
		$err = $db->getError () ;
		if ( $err['errno'] == 0 )
		{
			$cursor3 = $db_2->getSnapshot ( SDB_SNAP_DATABASE ) ;
			if ( !empty($cursor3) )
			{
				$arr3 = $cursor3->getNext() ;
				if ( $arr3['IsPrimary'] )
				{
					return true ;
				}
			}
			//$sum++;
		}
	}
	if ( $sum > 0 )
	{
		return true ;
	}
	else
	{
		return false ;
	}
}

function group_get_p_node( $db, $arr, $pid )
{
	$child_arr = $arr['Group'] ;
	while ( list( $key, $val ) = each ( $child_arr ) )
	{
		$arr2 = $child_arr[$key] ;
		if( $arr2['NodeID'] == $pid )
		{
			$child_child_arr = $arr2['Service'] ;
			while ( list( $key, $val ) = each ( $child_child_arr ) )
			{
				$arr3 = $child_child_arr[$key] ;
				if ( $arr3['Type'] == 0 )
				{
					return $arr2['HostName'].':'.$arr3['Name'];
				}
			}
		}
	}
	return "" ;
}

function group_count_node( $db, $arr )
{
	$sum = 0 ;
	$child_arr = $arr['Group'] ;
	while ( list( $key, $val ) = each ( $child_arr ) )
	{
		++$sum ;
	}
	return $sum ;
}

function db_count_cs( $db, $name )
{
	$sum = 0 ;
	$cursor = $db->listCollections () ;
	if ( !empty ( $cursor ) )
	{
		while ( $arr = $cursor->getNext() )
		{
			$str_temp = $arr['Name'] ;
			$arr_return =  explode ( ".", $str_temp ) ;
			if ($arr_return[0] == $name )
			{
				++$sum ;
			}
		}
	}
	return $sum ;
}

function isStart_2( $db, $groupname, $hostname, $servicename )
{
	$group = $db->selectGroup ( $groupname ) ;
	$err = $db->getError () ;
	if ( $err['errno'] != 0 )
	{
		return false ;
	}
	$node = $group->getNode ( $hostname.':'.$servicename ) ;
	$err = $db->getError () ;
	if ( $err['errno'] != 0 )
	{
		return false ;
	}
	$node->connect() ;
	$err = $db->getError () ;
	if ( $err['errno'] != 0 )
	{
		return false ;
	}
	return true ;
}

function is_object_array ( $arr )
{
	$num = 0 ;
	while ( list($key,$value) = each($arr) )
	{
		if ( $key != (string)$num )
		{
			return "object" ;
		}
		++$num ;
	}
	return "array" ;
}

function getValueType ( $arr, &$value, &$value_type )
{
	$value_type = gettype ( $arr ) ;
	if ( $value_type == "array" )
	{
		$value_type = is_object_array ( $arr ) ;
		if ( $value_type == "object" )
		{
			$value = "{...}" ;
		}
		else
		{
			$value = "[...]" ;
		}
	}
	else if ( $value_type == "object" )
	{
		$value_type = get_class ( $value ) ;
	}
	else if ( $value_type == "NULL" )
	{
		$value = "NULL" ;
	}
	else if ( $value_type == "boolean" )
	{
		$value_type = "bool" ;
		if ( $value )
		{
			$value = "true" ;
		}
		else
		{
			$value = "false" ;
		}
	}
	else if ( $value_type == "string" )
	{
		$value = '"'.$value.'"' ;
	}
	else if ( $value_type == "integer" )
	{
		$value_type = "int" ;
	}
	else
	{
		return ;
	}
}

function json2table ( $str_return, $width, $floors, $islast, $array_line, $str )
{
	$sum_key = count ( $str_return ) ;
	$sum = 0 ;
	$value_type = "" ;
	$temp_value = "";
	$isGK = false ;
	$temp_islast = $islast ;
	while ( list($key,$value) = each($str_return) )
	{
		$temp_value = $value ;
		getValueType ( $value, $temp_value, $value_type ) ;
		if ( $sum_key - 1 == $sum )
		{
			//第N层 的 最后一个
			if ( $floors > 0 )
			{
				if ( $islast == $floors && $value_type != "array" && $value_type != "object" )
				{
					$str = $str.'<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;">';
				}
				else
				{
					$str = $str.'<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom-color:#FFF;">';
				}
				if ( $value_type == "array" || $value_type == "object" )
				{
					$str = $str.'<div style="height:32px;float:left;width:'.(11+$floors*20).'px;margin-left:8px;"> <!-- 19 -->' ;
				}
				else
				{
					$str = $str.'<div style="height:32px;float:left;width:'.(12+$floors*20).'px;margin-left:8px;"> <!-- 19 -->' ;
				}
				$temp_num = 0 ;
				while ( $temp_num < $floors )
				{
					if ( $array_line[$temp_num] )
					{
						$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>' ;
					}
					else
					{
						$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;float:left;width:1px;"></div>' ;
					}
					++$temp_num;
				}
				$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:17px;margin-bottom:15px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
				<div style="height:17px;margin-top:15px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
				</div>
				<!-- 581 -->';
				if ( $value_type == "array" || $value_type == "object" )
				{
					$array_line[$floors] = false ;
					array_push ( $array_line, true ) ;
					if ( $isGK )
					{
						$isGK = false ;
						$str = $str.'<div style="width'.($width-19-$floors*20).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:1px solid #999;">
						<div style="width:'.(($width/3-29-$floors*20)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->' ;
					}
					else
					{
						$str = $str.'<div style="width'.($width-19-$floors*20).'px;height:32px;float:left;border-bottom:1px solid #999;">
						<div style="width:'.(($width/3-29-$floors*20)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->' ;
					}
					++$temp_islast ;
				}
				else
				{
					if ( $isGK )
					{
						$isGK = false ;
						$str = $str.'<div style="width'.($width-19-$floors*20).'px;height:32px;float:left;border-top:1px solid #999;">
						<div style="width:'.(($width/3-30-$floors*20)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->' ;
					}
					else
					{
						$str = $str.'<div style="width'.($width-19-$floors*20).'px;height:32px;float:left;">
						<div style="width:'.(($width/3-30-$floors*20)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->' ;
					}
				}
				$str = $str.'
						<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
						<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
					</div>
				</div>' ;
			}
			//第1层 的 最后一个
			else
			{
				++$temp_islast;
				if ( $value_type == "array" || $value_type == "object" )
				{
					$array_line[$floors] = false ;
					array_push ( $array_line, true ) ;
					if ( $isGK )
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:17px;margin-bottom:15px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:1px solid #999;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
					else
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:17px;margin-bottom:15px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
				}
				else
				{
					if ( $islast == $floors )
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:17px;margin-bottom:15px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;border-top:1px solid #999;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-24)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;

					}
					else
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:17px;margin-bottom:15px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">15 15 '.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-24)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
				}
			}
		}
		else
		{
			//有下列的
			if ( $value_type == "array" || $value_type == "object" )
			{
				//第N层 有下列
				if ( $floors > 0 )
				{
					array_push ( $array_line, true ) ;
					if ( $isGK )
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom-color:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:'.(13+$floors*19).'px;margin-left:8px;"> <!-- 19 -->';
							$temp_num = 0 ;
							while ( $temp_num <= $floors )
							{
								if ( $array_line[$temp_num] )
								{
									$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>' ;
								}
								else
								{
									$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;"></div>' ;
								}
								++$temp_num ;
							}
							$str = $str.'
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-20-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:1px solid #999;">
								<div style="width:'.(($width/3-31-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
					else
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom-color:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:'.(12+$floors*20).'px;margin-left:8px;"> <!-- 19 -->';
							$temp_num = 0 ;
							while ( $temp_num <= $floors )
							{
								if ( $array_line[$temp_num] )
								{
									$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>' ;
								}
								else
								{
									$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;"></div>' ;
								}
								++$temp_num ;
							}
							$str = $str.'
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-20-$floors*20).'px;height:32px;float:left;border-bottom:1px solid #999;">
								<div style="width:'.(($width/3-30-$floors*20)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
					$isGK = true ;
				}
				//第1层 有下列
				else
				{
					array_push ( $array_line, true ) ;
					if ( $isGK )
					{
						$str = $str.'
						<div class="div_table_1" class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom-color:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:1px solid #999;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;

					}
					else
					{
						$str = $str.'
						<div class="div_table_1" class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom-color:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
				}
			}
			else
			{
				//第N层 普通
				if ( $floors > 0 )
				{
					if ( $isGK )
					{
						$isGK = false ;
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom-color:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:'.(11+$floors*20).'px;margin-left:8px;"> <!-- 19 -->' ;
						$temp_num = 0 ;
						while ( $temp_num <= $floors )
						{
							if ( $array_line[$temp_num] )
							{
								$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>' ;
							}
							else
							{
								$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;"></div>' ;
							}
							++$temp_num ;
						}
						$str = $str.'<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-20-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:1px solid #999;">
								<div style="width:'.(($width/3-31-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
					else
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom-color:#FFF;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:'.(12+$floors*20).'px;margin-left:8px;"> <!-- 19 -->' ;
						$temp_num = 0 ;
						while ( $temp_num <= $floors )
						{
							if ( $array_line[$temp_num] )
							{
								$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>' ;
							}
							else
							{
								$str = $str.'<div style="margin-left:'.(($temp_num?1:0)*19).'px;height:32px;margin-bottom:0px;float:left;width:1px;"></div>' ;
							}
							++$temp_num ;
						}
						$str = $str.'<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*20).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:0px solid #999;">
								<div style="width:'.(($width/3-30-$floors*20)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
				}
				//第1层 普通
				else
				{
					if ( $isGK )
					{
						$isGK = false ;
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom:0px;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:1px solid #999;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}
					else
					{
						$str = $str.'
						<div class="div_table_1" style="width:'.$width.'px;border:1px solid #999;height:32px;border-top:0px;border-bottom:0px;"> <!-- 600 -->
							<!-- top -->
							<div style="height:32px;float:left;width:11px;margin-left:'.(8+$floors*19).'px;"> <!-- 19 -->
								<div style="height:32px;margin-bottom:0px;float:left;width:1px;background:url(../images/dot.png) repeat-y;"></div>
								<div style="height:32px;margin-top:16px;  float:left;width:10px;background:url(../images/dot_1.png) repeat-x;"></div>
							</div>
							<!-- 581 -->
							<div style="width'.($width-19-$floors*19).'px;height:32px;float:left;border-bottom:1px solid #999;border-top:0px solid #999;">
								<div style="width:'.(($width/3-29-$floors*19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$key.'</div> <!-- 180 - 10 -->
								<div style="width:'.(($width/2-10)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$temp_value.'</div> <!-- 300 - 10 -->
								<div style="width:'.(($width/6-19)).'px;float:left;height:20px;padding:6px;border-left:1px solid #999;">'.$value_type.'</div> <!-- 100 - 12 -->
							</div>
						</div>' ;
					}

				}
			}
		}
		if ( $value_type == "array" || $value_type == "object" )
		{
			$isGK = true ;
			$str = json2table ( $value, $width, $floors+1, $temp_islast, $array_line, $str ) ;
		}
		++$sum ;
	}
	return $str ;
}

?>
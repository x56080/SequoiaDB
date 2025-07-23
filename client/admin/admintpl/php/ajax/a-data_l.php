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

   Source File Name = a-data_l.php

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
$isfirst = true ;
//$cscl_list = '{"name":"数据库","child":{"name":"集合空间","child":[' ;
//$db -> install ( "{ install : false }" ) ;

$array_1 = array() ;
$array_2 = array() ;
$array_3 = array() ;

$cursor = $db -> getList ( SDB_LIST_COLLECTIONSPACES ) ;
if ( !empty ( $cursor ) )
{
	while ( $arr = $cursor -> getNext() )
	{
		array_push( $array_1, $arr ) ;
	}
}

$cursor = $db -> getSnapshot ( SDB_LIST_COLLECTIONSPACES ) ;
if ( !empty ( $cursor ) )
{
	while ( $arr = $cursor -> getNext() )
	{
        if( array_key_exists( 'Collection', $arr ) == true )
        {
           foreach( $arr['Collection'] as $index => $clInfo )
           {
              $clName = explode( '.', $clInfo['Name'] ) ;
              if( count( $clName ) > 1 )
              {
                 $arr['Collection'][$index]['Name'] = $clName[1] ;
              }
           }
        }
		array_push( $array_2, $arr ) ;
	}
}

$cursor = $db -> getSnapshot ( 8 ) ;
if ( !empty ( $cursor ) )
{
	while ( $arr = $cursor -> getNext() )
	{
        if( array_key_exists( 'IsMainCL', $arr ) && $arr['IsMainCL'] == true )
        {
            $tmpCSCL = explode( '.', $arr['Name'] ) ;
            array_push( $array_3, $tmpCSCL  ) ;
        }
	}
}

$cscl_list = arrayMerges( $array_1, $array_2 ) ;

foreach( $cscl_list as $key => $csInfo )
{
    if( array_key_exists( 'Collection', $cscl_list[ $key ] ) == false )
    {
        $cscl_list[ $key ]['Collection'] = [] ;
    }
    foreach( $array_3 as $masterInfo )
    {
        if( $csInfo['Name'] == $masterInfo[0] )
        {
            array_push( $cscl_list[ $key ]['Collection'], array( 'Name' => $masterInfo[1] ) ) ;
        }
    }
}

$cscl_list = json_encode( array( 'name' => '数据库', 'child' => array( 'name' => '集合空间', 'child' => $cscl_list ) ), true ) ;

$smarty -> assign( "cscl_list", $cscl_list ) ;

function arrayMerges( $a, $b )
{
   foreach( $a as $key => $value )
   {
      foreach( $b as $key2 => $value2 )
      {
         if( $value2['Name'] == $value['Name'] )
         {
            $a[$key] = array_merge( $a[$key], $b[$key2] ) ;
         }
      }
   }
   return $a ;
}

?>

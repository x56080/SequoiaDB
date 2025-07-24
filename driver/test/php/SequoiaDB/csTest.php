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

   Source File Name = csTest.php

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
class SequoiaDB_CS_Test extends PHPUnit_Framework_TestCase
{
   protected $hostname ;
   protected $port ;
   protected $address ;

   protected function setUp()
   {
      $this -> hostname = empty( $_POST['hostname'] ) ? '127.0.0.1' : $_POST['hostname'] ;
      $this -> port = empty( $_POST['port'] ) ? '11810' : $_POST['port'] ;
      $this -> address = $this -> hostname.':'.$this -> port ;
   }

   public function test_connect()
   {
      $err = sdbInitClient( array( 'enableCacheStrategy' => false ) ) ;
      $this -> assertEquals( 0, $err, '设置驱动缓存失败' ) ;
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->address.' )' ) ;
      return $db ;
   }
   
   /**
    * @depends test_connect
    */
   public function test_list_cs( $db )
   {
      $cursor = $db -> listCS() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'listCS错误' ) ;
      $this -> assertNotEmpty( $cursor, 'listCS错误' ) ;
      while( $record = $cursor -> next() )
      {
      }

      $cursor = $db -> listCSs() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'listCSs错误' ) ;
      $this -> assertNotEmpty( $cursor, 'listCSs错误' ) ;
      while( $record = $cursor -> next() )
      {
      }
   }

   /**
    * @depends test_connect
    */
   public function test_select_cs( $db )
   {
      $cs = $db -> selectCS( 'foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;
      
      $err = $cs -> drop() ;
      $this -> assertEquals( 0, $err['errno'], '删除cs错误' ) ;

      $cs = $db -> selectCS( 'foo_16384', 16384 ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;
      
      $cl = $cs -> selectCL( 'test', array( 'ReplSize' => -1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cl, '创建cl错误' ) ;

      $cursor = $db -> snapshot( SDB_SNAP_COLLECTIONSPACE ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取snapshot '.SDB_SNAP_COLLECTIONSPACE.' 错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取snapshot '.SDB_SNAP_COLLECTIONSPACE.' 错误' ) ;

      $pagesize = 0 ;
      while( $record = $cursor -> next() )
      {
         if( $record['Name'] == 'foo_16384' )
         {
            $pagesize = $record['PageSize'] ;
         }
      }
      $this -> assertEquals( 16384, $pagesize, '创建cs,设置options的PageSize为16384没有生效' ) ;

      $err = $cs -> drop() ;
      $this -> assertEquals( 0, $err['errno'], '删除cs错误' ) ;
   }

   /**
    * @depends test_connect
    */
   public function test_create_cs( $db )
   {
      $db -> dropCS( 'foo_8192' ) ;
      $err = $db -> createCS( 'foo_8192', array( 'PageSize' => 8192 ) ) ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      
      $cs = $db -> selectCS( 'foo_8192' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cs对象错误' ) ;
      $this -> assertNotEmpty( $cs, '获取cs对象错误' ) ;
      
      $cl = $cs -> selectCL( 'test', array( 'ReplSize' => -1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cl, '创建cl错误' ) ;

      $cursor = $db -> snapshot( SDB_SNAP_COLLECTIONSPACE ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取snapshot '.SDB_SNAP_COLLECTIONSPACE.' 错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取snapshot '.SDB_SNAP_COLLECTIONSPACE.' 错误' ) ;

      $pagesize = 0 ;
      while( $record = $cursor -> next() )
      {
         if( $record['Name'] == 'foo_8192' )
         {
            $pagesize = $record['PageSize'] ;
         }
      }
      $this -> assertEquals( 8192, $pagesize, '创建cs,设置options的PageSize为8192没有生效' ) ;

      $err = $cs -> drop() ;
      $this -> assertEquals( 0, $err['errno'], '删除cs错误' ) ;
   }

   /**
    * @depends test_connect
    */
   public function test_get_cs( $db )
   {
      $cs = $db -> selectCS( 'foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;

      $cs2 = $db -> getCS( 'foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cs错误' ) ;
      $this -> assertNotEmpty( $cs2, '获取cs错误' ) ;
   }

   /**
    * @depends test_connect
    */
   public function test_drop_cs( $db )
   {
      $cs = $db -> selectCS( 'foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;

      $err = $db -> dropCS( 'foo' ) ;
      $this -> assertEquals( 0, $err['errno'], '删除cs错误' ) ;

      $cs2 = $db -> getCS( 'foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( -34, $err['errno'], '删除cs错误' ) ;
      $this -> assertEmpty( $cs2, '删除cs错误' ) ;
   }
}
?>
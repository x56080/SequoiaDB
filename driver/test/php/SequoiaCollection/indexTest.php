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

   Source File Name = indexTest.php

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
class cl_index_Test extends PHPUnit_Framework_TestCase
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

   public function test_Connect()
   {
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      return $db ;
   }

   /**
    * @depends test_Connect
    */
   public function test_select_cs( $db )
   {
      $cs = $db -> selectCS( 'test_foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;
      return $cs ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    */
   public function test_select_cl( $db, $cs )
   {
      $cl = $cs -> selectCL( 'test_bar', array( 'ReplSize' => -1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cl错误' ) ;
      return $cl ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_createIndex_getIndex( $db, $cs, $cl )
   {
      $err = $cl -> createIndex( array( 'a' => 1 ), 'a', true, false, 128 ) ;
      $this -> assertEquals( 0, $err['errno'], '创建索引错误' ) ;
      
      $cursor = $cl -> getIndex( '' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取索引错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取索引错误' ) ;
      
      $cursor = $cl -> getIndex() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取索引错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取索引错误' ) ;
      
      $cursor = $cl -> getIndex( '$id' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取索引错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取索引错误' ) ;
      
      $cursor = $cl -> getIndex( 'a' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取索引错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取索引错误' ) ;
      $record = $cursor -> next() ;
      $this -> assertNotEmpty( $record, '获取索引错误' ) ;
      $this -> assertEquals( 'a', $record['IndexDef']['name'], '索引错误' ) ;
      $this -> assertEquals( 1, $record['IndexDef']['key']['a'], '索引错误' ) ;
      $this -> assertEquals( true, $record['IndexDef']['unique'], '索引错误' ) ;
      $this -> assertEquals( false, $record['IndexDef']['enforced'], '索引错误' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_dropIndex( $db, $cs, $cl )
   {
      $err = $cl -> dropIndex( "a" ) ;
      $this -> assertEquals( 0, $err['errno'], '删除索引错误' ) ;
      $this -> assertEquals( 0, $err['errno'], '删除索引错误' ) ;
      $cursor = $cl -> getIndex( 'a' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取索引错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取索引错误' ) ;
      $record = $cursor -> next() ;
      $this -> assertEmpty( $record, '索引没有删除' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_dropIdIndex( $db, $cs, $cl )
   {
      $err = $cl -> dropIdIndex() ;
      $this -> assertEquals( 0, $err['errno'], '删除id索引错误' ) ;
      $this -> assertEquals( 0, $err['errno'], '删除id索引错误' ) ;
      $cursor = $cl -> getIndex( '$id' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取索引错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取索引错误' ) ;
      $record = $cursor -> next() ;
      $this -> assertEmpty( $record, '索引没有删除' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_createIdIndex( $db, $cs, $cl )
   {
      $err = $cl -> createIdIndex() ;
      $this -> assertEquals( 0, $err['errno'], '创建id索引错误' ) ;
      $this -> assertEquals( 0, $err['errno'], '创建id索引错误' ) ;
      $cursor = $cl -> getIndex( '$id' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取索引错误' ) ;
      $this -> assertNotEmpty( $cursor, '获取索引错误' ) ;
      $record = $cursor -> next() ;
      $this -> assertNotEmpty( $record, '索引没有创建' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    */
   public function test_clear( $db, $cs )
   {
      $err = $cs -> drop() ;
      $this -> assertEquals( 0, $err['errno'], '删除cs错误' ) ;
   }
}
?>
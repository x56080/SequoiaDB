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

   Source File Name = clTest.php

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
class SequoiaDB_CL_Test extends PHPUnit_Framework_TestCase
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
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->address.' )' ) ;
      return $db ;
   }

   /**
    * @depends test_connect
    */
   public function test_list_cl( $db )
   {
      $cursor = $db -> listCL() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'listCS错误' ) ;
      $this -> assertNotEmpty( $cursor, 'listCS错误' ) ;
      while( $record = $cursor -> next() )
      {
      }
   }

   /**
    * @depends test_connect
    */
   public function test_get_cl( $db )
   {
      $cs = $db -> selectCS( 'foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;

      $cl = $cs -> selectCL( 'bar', array( 'ReplSize' => -1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cl, '创建cl错误' ) ;

      $cl = $db -> getCL( 'foo.bar' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cl错误' ) ;
      $this -> assertNotEmpty( $cl, '获取cl错误' ) ;
   }

   /**
    * @depends test_connect
    */
   public function test_truncate( $db )
   {
      $cs = $db -> selectCS( 'foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;

      $cl = $cs -> selectCL( 'bar', array( 'ReplSize' => -1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cl, '创建cl错误' ) ;

      $err = $cl -> insert( array( 'a' => 1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '插入记录错误' ) ;

      $err = $db -> truncate( 'foo.bar' ) ;
      $this -> assertEquals( 0, $err['errno'], '清空cl错误' ) ;

      $num = $cl -> count() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cl记录数错误' ) ;
      $this -> assertEquals( 0, $num, '获取cl记录数错误' ) ;

   }

}
?>
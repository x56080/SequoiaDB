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
class cs_Test extends PHPUnit_Framework_TestCase
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
      $err = sdbInitClient( array( 'enableCacheStrategy' => false ) ) ;
      $this -> assertEquals( 0, $err, '设置驱动缓存失败' ) ;
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->address.' )' ) ;
      return $db ;
   }

   /**
    * @depends test_Connect
    */
   public function test_select_cs( $db )
   {
      $cs = $db -> selectCS( 'test_cs' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;
      return $cs ;
   }

   /**
    * @depends test_Connect
    */
   public function test_drop( $db )
   {
      $cs = $db -> selectCS( 'drop_foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;

      $err = $cs -> drop() ;
      $this -> assertEquals( 0, $err['errno'], '删除cl错误' ) ;

      $err = $cs -> drop() ;
      $this -> assertEquals( -34, $err['errno'], '删除cs错误' ) ;

      $cl = $cs -> selectCL( 'bar', array( 'ReplSize' => -1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( -34, $err['errno'], '创建cl错误' ) ;
      $this -> assertEmpty( $cl, '创建cl错误' ) ;

      $cs = $db -> getCS( 'drop_foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( -34, $err['errno'], '获取cs错误' ) ;
      $this -> assertEmpty( $cs, '获取cs错误' ) ;
      
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    */
   public function test_getName( $db, $cs )
   {      
      $name = $cs -> getName() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cs名字错误' ) ;
      $this -> assertEquals( 'test_cs', $name, '获取cs名字错误' ) ;
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

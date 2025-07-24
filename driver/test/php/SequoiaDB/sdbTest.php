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

   Source File Name = sdbTest.php

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
class SequoiaDB_Test extends PHPUnit_Framework_TestCase
{
   protected $hostname ;
   protected $port ;
   protected $address ;
   protected $arrayAddress ;

   protected function setUp()
   {
      $this -> hostname = empty( $_POST['hostname'] ) ? '127.0.0.1' : $_POST['hostname'] ;
      $this -> port = empty( $_POST['port'] ) ? '11810' : $_POST['port'] ;
      $this -> address = $this -> hostname.':'.$this -> port ;
      $this -> arrayAddress = array( $this -> address ) ;
   }

   public function test_new_SequoiaDB_to_connect()
   {     
      //hostname:port
      $db = new SequoiaDB( $this->address );
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      $db -> close() ;
      unset( $db ) ;

      //array( hostname, hostname:post )
      $db = new SequoiaDB( $this->arrayAddress );
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->arrayAddress[0].' )' ) ;
      $db -> close() ;
      unset( $db ) ;
   }

   public function test_connect()
   {
      //hostname:port
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      $db -> close() ;
      unset( $db ) ;

      //array( hostname, hostname:post )
      $db = new SequoiaDB();
      $err = $db -> connect( $this->arrayAddress ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->arrayAddress[0].' )' ) ;
      return $db ;
   }

   public function test_close()
   {
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      $db -> close() ;
      $db -> snapshot( SDB_SNAP_CONTEXTS ) ;
      $err = $db -> getError() ;
      $this -> assertTrue( $err['errno'] == -75 || $err['errno'] == -64 ) ;
      unset( $db ) ;
   }

   /**
    * @depends test_connect
    */
   public function test_install( $db )
   {
      $db -> install( array( 'install' => false ) ) ;
      $err = $db -> getError() ;
      $this -> assertTrue( is_string( $err ), 'install设置返回json字符串类型失败' ) ;

      $db -> install( array( 'install' => true ) ) ;
      $err = $db -> getError() ;
      $this -> assertTrue( is_array( $err ), 'install设置返回数组类型失败' ) ;
   }
   
   /**
    * @depends test_connect
    */
   public function test_getError( $db )
   {
      $cursor = $db -> snapshot( 0 ) ;
      $this -> assertNotEmpty( $cursor, '获取快照失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'] ) ;
      
      $cursor = $db -> snapshot( 999 ) ;
      $this -> assertEmpty( $cursor, '获取快照应该失败的,但是没有返回NULL的值' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( -6, $err['errno'] ) ;
   }
   
   public function test_initClient()
   {
      $err = sdbInitClient( array( 'enableCacheStrategy' => false ) ) ;
      $this -> assertEquals( 0, $err, '设置驱动缓存失败' ) ;
      
      $db1 = new SequoiaDB();
      $err = $db1 -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      
      $time1 = 0 ;
      $time2 = 0 ;
      
      $start = microtime( true ) ;
      for( $i = 0; $i < 10000; ++$i )
      {
         $cs = $db1 -> selectCS( 'test_init_client' ) ;
         $err = $db1 -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
         $this -> assertNotEmpty( $cs, '创建cs错误' ) ;
         
         $cl = $cs -> selectCL( 'test_init_client' ) ;
         $err = $db1 -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cs错误' ) ;
      }
      $end = microtime( true ) ;
      $time1 = round( $end - $start, 3 ) ;
      
      $err = sdbInitClient( array( 'enableCacheStrategy' => true, 'cacheTimeInterval' => 300 ) ) ;
      $this -> assertEquals( 0, $err, '设置驱动缓存失败' ) ;
      
      $db2 = new SequoiaDB();
      $err = $db2 -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      
      $start = microtime( true ) ;
      for( $i = 0; $i < 10000; ++$i )
      {
         $cs = $db2 -> selectCS( 'test_init_client' ) ;
         $err = $db2 -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
         $this -> assertNotEmpty( $cs, '创建cs错误' ) ;
         
         $cl = $cs -> selectCL( 'test_init_client' ) ;
         $err = $db2 -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cs错误' ) ;
      }
      $end = microtime( true ) ;
      $time2 = round( $end - $start, 3 ) ;
      
      $this -> assertLessThan( $time1, $time2, 'sdbInitCLient没有生效' ) ;
   }
   
   /**
    * @depends test_connect
    */
   public function test_isValid( $db )
   {
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误, 数据库地址: '.$this->address ) ;
      
      $valid = $db -> isValid() ;
      $this -> assertEquals( true, $valid, '数据库没有正常连接, 数据库地址: '.$this->address ) ;
      
      $db -> close() ;

      $valid = $db -> isValid() ;
      $this -> assertEquals( false, $valid, '数据库没有断开连接, 数据库地址: '.$this->address ) ;
   }

   /**
    * @depends test_connect
    */
   public function test_snapshot( $db )
   {
      for( $i = 0; $i < 10; ++ $i )
      {
         $cursor = $db -> snapshot( $i ) ;
         $err = $db -> getError() ;
         if( $err['errno'] == -159 )
         {
            continue ;
         }
         if( $err['errno'] == -29 )
         {
            continue ;
         }
         $this -> assertEquals( 0, $err['errno'], '测试snapshot接口, 快照类型: '.$i ) ;
         $this -> assertNotEmpty( $cursor, '测试snapshot接口' ) ;
         while( $record = $cursor -> next() )
         {
         }
      }
      for( $i = 0; $i < 10; ++ $i )
      {
         $cursor = $db -> getSnapshot( $i ) ;
         $err = $db -> getError() ;
         if( $err['errno'] == -159 )
         {
            continue ;
         }
         if( $err['errno'] == -29 )
         {
            continue ;
         }
         $this -> assertEquals( 0, $err['errno'], '测试getSnapshot接口, 快照类型: '.$i ) ;
         $this -> assertNotEmpty( $cursor, '测试getSnapshot接口' ) ;
         while( $record = $cursor -> next() )
         {
         }
      }
   }

   /**
    * @depends test_connect
    */
   public function test_list( $db )
   {
      for( $i = 0; $i < 12; ++ $i )
      {
         $cursor = $db -> list( $i ) ;
         $err = $db -> getError() ;
         if( $err['errno'] == -159 )
         {
            continue ;
         }
         $this -> assertEquals( 0, $err['errno'], '测试list接口' ) ;
         $this -> assertNotEmpty( $cursor, '测试list接口' ) ;
         while( $record = $cursor -> next() )
         {
         }

         $cursor = $db -> getList( $i ) ;
         $err = $db -> getError() ;
         if( $err['errno'] == -159 )
         {
            continue ;
         }
         $this -> assertEquals( 0, $err['errno'], '测试getList接口' ) ;
         $this -> assertNotEmpty( $cursor, '测试getList接口' ) ;
         while( $record = $cursor -> next() )
         {
         }
      }
   }

   /**
    * @depends test_connect
    */
   public function test_reset_snapshot( $db )
   {
      $cursor = $db -> snapshot( SDB_SNAP_DATABASE ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'reset前获取snapshot '.SDB_SNAP_DATABASE.' 错误' ) ;
      $this -> assertNotEmpty( $cursor, 'reset前获取snapshot '.SDB_SNAP_DATABASE.' 错误' ) ;
      $old = $cursor -> next() ;
      $this -> assertNotEmpty( $old, 'reset前获取snapshot '.SDB_SNAP_DATABASE.' 错误' ) ;

      $err = $db -> resetSnapshot() ;

      $this -> assertEquals( 0, $err['errno'], 'resetSnapshot错误' ) ;
      $cursor = $db -> snapshot( SDB_SNAP_DATABASE ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'reset后获取snapshot '.SDB_SNAP_DATABASE.' 错误' ) ;
      $this -> assertNotEmpty( $cursor, 'reset后获取snapshot '.SDB_SNAP_DATABASE.' 错误' ) ;
      $new = $cursor -> next() ;
      $this -> assertNotEmpty( $new, 'reset后获取snapshot '.SDB_SNAP_DATABASE.' 错误' ) ;
      
      if( array_key_exists( 'replNetIn', $old ) )
      {
         $this -> assertLessThan( $old['replNetIn'], $new['replNetIn'], 'resetSnapshot没有生效' ) ;
      }
      else
      {
         $this -> assertLessThan( $old['svcNetIn'], $new['svcNetIn'], 'resetSnapshot没有生效' ) ;
      }
   }

}
?>
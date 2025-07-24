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

   Source File Name = cursorTest.php

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
class cursor_Test extends PHPUnit_Framework_TestCase
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
      $db = new Sequoiadb();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->address.' )' ) ;
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
   public function test_next_current( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      $err = $cl -> insert( array( '_id' => new SequoiaID( '123456789012345678901234' ), 'date' => new SequoiaDate( '1991-11-27' ), 'timestamp' => new SequoiaTimestamp( '1991-11-27-12.30.20.123456' ), 'regex' => new SequoiaRegex( 'a', 'i' ), 'binary' => new SequoiaBinary( 'aGVsbG8=', '1' ), 'minKey' => new SequoiaMinKey(), 'maxKey' => new SequoiaMaxKey() ) ) ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      $id = $err['_id'] ;
      $this -> assertEquals( 24, strlen( $id ), '插入记录失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;

      $cursor = $cl -> find() ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      $record = $cursor -> next() ;
      $this -> assertNotEmpty( $record, 'next记录失败' ) ;
      $this -> assertTrue( is_object( $record['_id'] )       && is_a( $record['_id'],       'SequoiaID' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['date'] )      && is_a( $record['date'],      'SequoiaDate' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['timestamp'] ) && is_a( $record['timestamp'], 'SequoiaTimestamp' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['regex'] )     && is_a( $record['regex'],     'SequoiaRegex' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['binary'] )    && is_a( $record['binary'],    'SequoiaBinary' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['minKey'] )    && is_a( $record['minKey'],    'SequoiaMinKey' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['maxKey'] )    && is_a( $record['maxKey'],    'SequoiaMaxKey' ), '插入记录失败' ) ;
      $this -> assertEquals( $id, $record['_id'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '1991-11-27', $record['date'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '1991-11-27-12.30.20.123456', $record['timestamp'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '/a/i', $record['regex'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '(1)aGVsbG8=', $record['binary'] -> __toString(), '插入记录失败' ) ;

      $record = $cursor -> current() ;
      $this -> assertNotEmpty( $record, 'cursor记录失败' ) ;
      $this -> assertTrue( is_object( $record['_id'] )       && is_a( $record['_id'],       'SequoiaID' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['date'] )      && is_a( $record['date'],      'SequoiaDate' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['timestamp'] ) && is_a( $record['timestamp'], 'SequoiaTimestamp' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['regex'] )     && is_a( $record['regex'],     'SequoiaRegex' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['binary'] )    && is_a( $record['binary'],    'SequoiaBinary' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['minKey'] )    && is_a( $record['minKey'],    'SequoiaMinKey' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['maxKey'] )    && is_a( $record['maxKey'],    'SequoiaMaxKey' ), '插入记录失败' ) ;
      $this -> assertEquals( $id, $record['_id'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '1991-11-27', $record['date'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '1991-11-27-12.30.20.123456', $record['timestamp'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '/a/i', $record['regex'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '(1)aGVsbG8=', $record['binary'] -> __toString(), '插入记录失败' ) ;

      $cursor = $cl -> find() ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      $db -> install( array( 'install' => false ) ) ;
      
      $record = $cursor -> next() ;
      $this -> assertNotEquals( 0, strlen( $record ), 'cursor记录失败' ) ;
      $this -> assertEquals( '{ "_id": { "$oid": "123456789012345678901234" }, "date": { "$date": "1991-11-27" }, "timestamp": { "$timestamp": "1991-11-27-12.30.20.123456" }, "regex": { "$regex": "a", "$options": "i" }, "binary": { "$binary": "aGVsbG8=", "$type" : "1" }, "minKey": { "$minKey": 1 }, "maxKey": { "$maxKey": 1 } }', $record, 'cursor记录失败' ) ;
      
      $record = $cursor -> current() ;
      $this -> assertNotEquals( 0, strlen( $record ), 'cursor记录失败' ) ;
      $this -> assertEquals( '{ "_id": { "$oid": "123456789012345678901234" }, "date": { "$date": "1991-11-27" }, "timestamp": { "$timestamp": "1991-11-27-12.30.20.123456" }, "regex": { "$regex": "a", "$options": "i" }, "binary": { "$binary": "aGVsbG8=", "$type" : "1" }, "minKey": { "$minKey": 1 }, "maxKey": { "$maxKey": 1 } }', $record, 'cursor记录失败' ) ;
   }
   
   /**
    * @depends test_Connect
    */
   public function test_install( $db )
   {
      $db -> install( array( 'install' => true ) ) ;
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

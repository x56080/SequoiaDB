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

   Source File Name = recordTest.php

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
class cl_record_Test extends PHPUnit_Framework_TestCase
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
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      return $db ;
   }
   
   /**
    * @depends test_Connect
    */
   public function test_isStandlone( $db )
   {
      $cursor = $db -> list( SDB_LIST_SHARDS ) ;
      $err = $db -> getError() ;
      if( $err['errno'] == -159 )
      {
         return true ;
      }
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cursor, '创建cl错误' ) ;
      return false ;
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
    * @depends test_isStandlone
    */
   public function test_select_cl( $db, $cs, $isStandlone )
   {
      $cl = array() ;
      if( $isStandlone )
      {
         $cl = $cs -> selectCL( 'test_bar' ) ;
      }
      else
      {
         $cl = $cs -> selectCL( 'test_bar', array( 'ReplSize' => -1 ) ) ;
      }
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
      return $cl ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_insert( $db, $cs, $cl )
   {
      $id = '' ;
      $err = $cl -> insert( array( 'bool' => true,
                                   'null' => null,
                                   'int' => 2147483647,
                                   'long' => new SequoiaINT64( '214748364800' ),
                                   'double' => 1.1,
                                   'string' => 'hello',
                                   'array' => array( 1, 2, 3 ),
                                   'object' => array( 'a' => 1, 'b' => 2 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      $id = $err['_id'] ;
      $this -> assertEquals( 24, strlen( $id ), '插入记录失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;

      $cursor = $cl -> find() ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $record = $cursor -> next() ;
      $this -> assertEquals( $id, $record['_id'] -> __toString(), '插入记录失败' ) ;
      $this -> assertTrue( is_bool( $record['bool'] ), '插入记录失败' ) ;
      $this -> assertTrue( is_null( $record['null'] ), '插入记录失败' ) ;
      $this -> assertTrue( is_int( $record['int'] ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['long'] ) && is_a( $record['long'], 'SequoiaINT64' ), '插入记录失败' ) ;
      $this -> assertTrue( is_float( $record['double'] ), '插入记录失败' ) ;
      $this -> assertTrue( is_string( $record['string'] ), '插入记录失败' ) ;
      $this -> assertTrue( is_array( $record['array'] ), '插入记录失败' ) ;
      $this -> assertTrue( is_array( $record['object'] ), '插入记录失败' ) ;

      $err = $cl -> insert( $record ) ;
      $this -> assertEquals( -38, $err['errno'], '插入记录失败' ) ;

      $err = $cl -> insert( array( '_id' => new SequoiaID( '123456789012345678901234' ),
                                   'date' => new SequoiaDate( '1991-11-27' ),
                                   'timestamp' => new SequoiaTimestamp( '1991-11-27-12.30.20.123456' ),
                                   'regex' => new SequoiaRegex( 'a', 'i' ),
                                   'binary' => new SequoiaBinary( 'aGVsbG8=', '1' ),
                                   'minKey' => new SequoiaMinKey(),
                                   'maxKey' => new SequoiaMaxKey(),
                                   'decimal' => new SequoiaDecimal( '10.000000109', 10, 8 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      $id = $err['_id'] ;
      $this -> assertEquals( 24, strlen( $id ), '插入记录失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      $cursor = $cl -> find( array( '_id' => new SequoiaID( '123456789012345678901234' ) ) ) ;
      $record = $cursor -> next() ;
      $this -> assertNotEmpty( $record, 'next记录失败' ) ;
      $this -> assertTrue( is_object( $record['_id'] )       && is_a( $record['_id'],       'SequoiaID' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['date'] )      && is_a( $record['date'],      'SequoiaDate' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['timestamp'] ) && is_a( $record['timestamp'], 'SequoiaTimestamp' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['regex'] )     && is_a( $record['regex'],     'SequoiaRegex' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['binary'] )    && is_a( $record['binary'],    'SequoiaBinary' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['minKey'] )    && is_a( $record['minKey'],    'SequoiaMinKey' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['maxKey'] )    && is_a( $record['maxKey'],    'SequoiaMaxKey' ), '插入记录失败' ) ;
      $this -> assertTrue( is_object( $record['decimal'] )   && is_a( $record['decimal'],   'SequoiaDecimal' ), '插入记录失败' ) ;
      $this -> assertEquals( $id, $record['_id'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '1991-11-27', $record['date'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '1991-11-27-12.30.20.123456', $record['timestamp'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '/a/i', $record['regex'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '(1)aGVsbG8=', $record['binary'] -> __toString(), '插入记录失败' ) ;
      $this -> assertEquals( '10.00000011', $record['decimal'] -> __toString(), '插入记录失败' ) ;
      
      $dollCmd = array( '{ a : { $addtoset : 1 } }',
                        '{ a : { $all : 1 } }',
                        '{ a : { $and : 1 } }',
                        '{ a : { $bitand : 1 } }',
                        '{ a : { $bitor : 1 } }',
                        '{ a : { $bitnot : 1 } }',
                        '{ a : { $bitxor : 1 } }',
                        '{ a : { $bit : 1 } }',
                        '{ a : { $elemMatch : 1 } }',
                        '{ a : { $exists : 1 } }',
                        '{ a : { $isnull : 1 } }',
                        '{ a : { $gte : 1 } }',
                        '{ a : { $gt : 1 } }',
                        '{ a : { $inc : 1 } }',
                        '{ a : { $lte : 1 } }',
                        '{ a : { $lt : 1 } }',
                        '{ a : { $in : 1 } }',
                        '{ a : { $et : 1 } }',
                        '{ a : { $maxDistance : 1 } }',
                        '{ a : { $mod : 1 } }',
                        '{ a : { $near : 1 } }',
                        '{ a : { $ne : 1 } }',
                        '{ a : { $nin : 1 } }',
                        '{ a : { $not : 1 } }',
                        '{ a : { $options : 1 } }',
                        '{ a : { $or : 1 } }',
                        '{ a : { $pop : 1 } }',
                        '{ a : { $pull_all : 1 } }',
                        '{ a : { $pull : 1 } }',
                        '{ a : { $push_all : 1 } }',
                        '{ a : { $push : 1 } }',
                        '{ a : { $regex : "123", $options: "i" } }',
                        '{ a : { $rename : 1 } }',
                        '{ a : { $replace : 1 } }',
                        '{ a : { $set : 1 } }',
                        '{ a : { $size : 1 } }',
                        '{ a : { $type : 1 } }',
                        '{ a : { $unset : 1 } }',
                        '{ a : { $within : 1 } }',
                        '{ a : { $field : 1 } }',
                        '{ a : { $sum : 1 } }',
                        '{ a : { $project : 1 } }',
                        '{ a : { $match : 1 } }',
                        '{ a : { $limit : 1 } }',
                        '{ a : { $skip : 1 } }',
                        '{ a : { $group : 1 } }',
                        '{ a : { $first : 1 } }',
                        '{ a : { $last : 1 } }',
                        '{ a : { $max : 1 } }',
                        '{ a : { $min : 1 } }',
                        '{ a : { $avg : 1 } }',
                        '{ a : { $sort : 1 } }',
                        '{ a : { $mergearrayset : 1 } }',
                        '{ a : { $cast : 1 } }',
                        '{ a : { $include : 1 } }',
                        '{ a : { $default : 1 } }',
                        '{ a : { $elemMatchOne : 1 } }',
                        '{ a : { $slice : 1 } }',
                        '{ a : { $abs : 1 } }',
                        '{ a : { $ceiling : 1 } }',
                        '{ a : { $floor : 1 } }',
                        '{ a : { $add : 1 } }',
                        '{ a : { $subtract : 1 } }',
                        '{ a : { $multiply : 1 } }',
                        '{ a : { $divide : 1 } }',
                        '{ a : { $substr : 1 } }',
                        '{ a : { $strlen : 1 } }',
                        '{ a : { $lower : 1 } }',
                        '{ a : { $upper : 1 } }',
                        '{ a : { $ltrim : 1 } }',
                        '{ a : { $rtrim : 1 } }',
                        '{ a : { $trim : 1 } }',
                        '{ a : { $count : 1 } }',
                        '{ a : { $Meta : 1 } }',
                        '{ a : { $Aggr : 1 } }',
                        '{ a : { $SetOnInsert : 1 } }',
                        '{ a : { $Modify : 1 } }' ) ;
      foreach( $dollCmd as $key => $value )
      {
         $err = $cl -> insert( $value ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败 '.$value ) ;
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_bulkInsert( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      $err = $cl -> bulkInsert( array( 'a' => 1 ) ) ;
      $this -> assertEquals( 0, $err['errno'], '批量插入记录失败' ) ;
      
      $err = $cl -> bulkInsert( '{a:2}' ) ;
      $this -> assertEquals( 0, $err['errno'], '批量插入记录失败' ) ;

      $records = array(
         array( 'a' => 3 ),
         array( 'a' => 4 ),
         array( 'a' => 5 ),
         array( 'a' => 6 ),
         array( 'a' => 7 ),
         '{ a: 8 }',
         '{ a: 9 }',
         '{ a: 10 }',
         '{ a: 11 }',
         '{ _id: 1, a: 12 }'
      ) ;
      $err = $cl -> bulkInsert( $records ) ;
      $this -> assertEquals( 0, $err['errno'], '批量插入记录失败' ) ;
      
      $records = array(
         '{ _id: 1, a: 13 }',
         '{ a: 14 }'
      ) ;
      $err = $cl -> bulkInsert( $records, SDB_FLG_INSERT_CONTONDUP ) ;
      $this -> assertEquals( 0, $err['errno'], '批量插入记录失败' ) ;
      
      $err = $cl -> bulkInsert( $records ) ;
      $this -> assertEquals( -38, $err['errno'], '批量插入记录失败' ) ;
      
      $count = $cl -> count() ;
      $this -> assertEquals( 13, $count, '获取记录数有问题' ) ;

      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_remove( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
      
      $count = $cl -> count() ;
      $this -> assertEquals( 0, $count, '删除记录失败' ) ;
      
      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      for( $i = 101; $i <= 200; ++$i )
      {
         $err = $cl -> insert( '{a:'.$i.'}' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      
      $err = $cl -> remove( array( 'a' => array( '$lte' => 50 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      $count = $cl -> count() ;
      $this -> assertEquals( 150, $count, '删除记录失败' ) ;
      
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
      
      $count = $cl -> count() ;
      $this -> assertEquals( 0, $count, '删除记录失败' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_update( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      for( $i = 101; $i <= 200; ++$i )
      {
         $err = $cl -> insert( '{a:'.$i.'}' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      
      $err = $cl -> update( '{ $set: { b : 1 } }', array( 'a' => array( '$gt' => 100 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      
      $count = 0 ;
      $cursor = $cl -> find() ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      while( $record = $cursor -> next() )
      {
         if( $record['a'] <= 100 )
         {
            $this -> assertFalse( array_key_exists( 'b', $record ), '更新操作有问题' ) ;
         }
         else if( $record['a'] > 100 )
         {
            $this -> assertTrue( array_key_exists( 'b', $record ), '更新操作有问题' ) ;
            $this -> assertEquals( 1, $record['b'], '更新操作有问题' ) ;
         }
         ++$count ;
      }
      $this -> assertEquals( 200, $count, '查询的记录有问题' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_upsert( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      for( $i = 101; $i <= 200; ++$i )
      {
         $err = $cl -> insert( '{a:'.$i.'}' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      
      $err = $cl -> upsert( '{ $set: { b : 1 } }', array( 'a' => array( '$gt' => 100 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], 'upsert记录失败' ) ;
      
      $err = $cl -> upsert( '{ $set: { a : 201 } }', array( 'a' => array( '$gt' => 200 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], 'upsert记录失败' ) ;
      
      $count = 0 ;
      $cursor = $cl -> find() ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      $upsert2Num = 0 ;
      while( $record = $cursor -> next() )
      {
         if( $record['a'] <= 100 )
         {
            $this -> assertFalse( array_key_exists( 'b', $record ), '更新操作有问题' ) ;
         }
         else if( $record['a'] > 100 && $record['a'] <= 200 )
         {
            $this -> assertTrue( array_key_exists( 'b', $record ), '更新操作有问题' ) ;
            $this -> assertEquals( 1, $record['b'], '更新操作有问题' ) ;
         }
         else if( $record['a'] == 201 )
         {
            ++$upsert2Num ;
         }
         ++$count ;
      }
      $this -> assertEquals( 1, $upsert2Num, '查询的记录有问题' ) ;
      $this -> assertEquals( 201, $count, '查询的记录有问题' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_find( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      for( $i = 101; $i <= 200; ++$i )
      {
         $err = $cl -> insert( '{a:'.$i.'}' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }

      $count = 0 ;
      $cursor = $cl -> find() ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      while( $record = $cursor -> next() )
      {
         ++$count ;
      }
      $this -> assertEquals( 200, $count, '查询的记录有问题' ) ;

      $count = 0 ;
      $cursor = $cl -> find( array( 'a' => array( '$lte' => 50 ) ) ) ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      while( $record = $cursor -> next() )
      {
         ++$count ;
      }
      $this -> assertEquals( 50, $count, '查询的记录有问题' ) ;
      
      $cursor = $cl -> find( null, array( 'b' => 123 ) ) ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      while( $record = $cursor -> next() )
      {
         $this -> assertEquals( 123, $record['b'], '查询的记录有问题' ) ;
      }
      
      $count = 0 ;
      $cursor = $cl -> find( null, null, array( 'a' => -1 ) ) ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      while( $record = $cursor -> next() )
      {
         $this -> assertEquals( 200 - $count, $record['a'], '查询的记录有问题' ) ;
         ++$count ;
      }

      $count = 0 ;
      $cursor = $cl -> find( null, null, null, null, 200 ) ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      while( $record = $cursor -> next() )
      {
         ++$count ;
      }
      $this -> assertEquals( 0, $count, '查询的记录有问题' ) ;

      $count = 0 ;
      $cursor = $cl -> find( null, null, null, null, 10, 10 ) ;
      $this -> assertNotEmpty( $cursor, '查询失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '查询失败' ) ;
      while( $record = $cursor -> next() )
      {
         ++$count ;
      }
      $this -> assertEquals( 10, $count, '查询的记录有问题' ) ;

      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_findAndUpdate( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      
      $cursor = $cl -> findAndUpdate( array( '$set' => array( 'a' => 0 ) ), array( 'a' => array( '$gt' => 0 ) ) ) ;
      $this -> assertNotEmpty( $cursor, 'findAndUpdate失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'findAndUpdate失败' ) ;
      while( $record = $cursor -> next() )
      {
         $this -> assertGreaterThan( 0, $record['a'], 'findAndUpdate的记录有问题' ) ;
      }
      
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
      
      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      $cursor = $cl -> findAndUpdate( array( '$set' => array( 'a' => 0 ) ), array( 'a' => array( '$gt' => 0 ) ), null, null, null, 0, -1, 0, true ) ;
      $this -> assertNotEmpty( $cursor, 'findAndUpdate失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'findAndUpdate失败' ) ;
      while( $record = $cursor -> next() )
      {
         $this -> assertEquals( 0, $record['a'], 'findAndUpdate的记录有问题' ) ;
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_findAndRemove( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      
      $count = 0 ;
      $cursor = $cl -> findAndRemove( array( 'a' => array( '$gt' => 0 ) ), null, null, null, 0, -1, 0 ) ;
      $this -> assertNotEmpty( $cursor, 'findAndRemove失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'findAndRemove失败' ) ;
      while( $record = $cursor -> next() )
      {
         ++$count ;
      }
      $this -> assertEquals( 100, $count, 'findAndRemove失败' ) ;
      
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_explain( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }

      $count = 0 ;
      $cursor = $cl -> explain( array( 'a' => array( '$gt' => 0 ) ), null, null, null, 0, -1, 0, null ) ;
      $this -> assertNotEmpty( $cursor, 'explain失败' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'explain失败' ) ;
      while( $record = $cursor -> next() )
      {
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_aggregate( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;

      $err = $cl -> insert( array( 'id'   => 0,
                                   'name' => 'A',
                                   'age'  => 16,
                                   'info' => array( 'a' => 1, 'b' => 2, 'c' => 3 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      $err = $cl -> insert( array( 'id'   => 1,
                                   'name' => 'B',
                                   'age'  => 17,
                                   'info' => array( 'a' => 4, 'b' => 5, 'c' => 6 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      $err = $cl -> insert( array( 'id'   => 2,
                                   'name' => 'C',
                                   'age'  => 18,
                                   'info' => array( 'a' => 7, 'b' => 8, 'c' => 9 ) ) ) ;
      $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      
      $cursor = $cl -> aggregate( array( '{ $match: { id: { $gt: 0 } } }', '{ $project: { id: 1, name: 1, info.a: 1 } }' ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'aggregate失败' ) ;
      $this -> assertNotEmpty( $cursor, 'aggregate失败' ) ;
      while( $record = $cursor -> next() )
      {
         if( $record['id'] == 1 )
         {
            $this -> assertEquals( 'B', $record['name'], 'aggregate失败' ) ;
            $this -> assertEquals( 4, $record['info.a'], 'aggregate失败' ) ;
         }
         else if( $record['id'] == 2 )
         {
            $this -> assertEquals( 'C', $record['name'], 'aggregate失败' ) ;
            $this -> assertEquals( 7, $record['info.a'], 'aggregate失败' ) ;
         }
         else
         {
            $this -> assertEquals( 1, $record['id'], 'aggregate失败' ) ;
            $this -> assertEquals( 2, $record['id'], 'aggregate失败' ) ;
         }
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_count( $db, $cs, $cl )
   {
      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
      
      $count = $cl -> count() ;
      $this -> assertEquals( 0, $count, '删除记录失败' ) ;
      
      for( $i = 1; $i <= 100; ++$i )
      {
         $err = $cl -> insert( array( 'a' => $i ) ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }
      for( $i = 101; $i <= 200; ++$i )
      {
         $err = $cl -> insert( '{a:'.$i.'}' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录失败' ) ;
      }

      $count = $cl -> count() ;
      $this -> assertEquals( 200, $count, '获取记录数有问题' ) ;

      $count = $cl -> count( array( 'a' => array( '$lte' => 50 ) ) ) ;
      $this -> assertEquals( 50, $count, '获取记录数有问题' ) ;

      $err = $cl -> remove() ;
      $this -> assertEquals( 0, $err['errno'], '删除记录失败' ) ;
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

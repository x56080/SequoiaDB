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

   Source File Name = domainTest.php

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
class SequoiaDB_Domain_Test extends PHPUnit_Framework_TestCase
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
   public function test_isStandlone( $db )
   {
      $cursor = $db -> list( SDB_LIST_SHARDS ) ;
      $err = $db -> getError() ;
      if( $err['errno'] == -159 )
      {
         return true ;
      }
      $this -> assertEquals( 0, $err['errno'], 'list错误' ) ;
      $this -> assertNotEmpty( $cursor, 'list错误' ) ;
      return false ;
   }
   
   /**
    * @depends test_connect
    * @depends test_isStandlone
    */
   public function test_getGroupList( $db, $isStandlone )
   {
      if( $isStandlone == false )
      {
         $groupList = array() ;
         $cursor = $db -> list( SDB_LIST_GROUPS, '{ $and: [ { GroupName:{ $ne: "SYSCatalogGroup" } }, { GroupName: { $ne: "SYSCoord" } } ] }', array( 'GroupName' => 1 ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '获取group列表错误' ) ;
         $this -> assertNotEmpty( $cursor, '获取group列表错误' ) ;
         while( $record = $cursor -> next() )
         {
            array_push( $groupList, $record['GroupName'] ) ;
         }
         return $groupList ;
      }
      else
      {
         return array();
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_isStandlone
    * @depends test_getGroupList
    */
   public function test_createDomain( $db, $isStandlone, $groupList )
   {
      if( $isStandlone == false )
      {
         if( count( $groupList ) < 2 )
         {
            return ;
         }
         $err = $db -> createDomain( 'php_driver_my_domain', array( 'Groups' => $groupList, 'AutoSplit' => true ) ) ;
         $this -> assertEquals( 0, $err['errno'], 'createDomain错误' ) ;
      }
   }

   /**
    * @depends test_connect
    * @depends test_isStandlone
    * @depends test_getGroupList
    */
   public function test_listDomain( $db, $isStandlone, $groupList )
   {
      if( $isStandlone == false )
      {
         if( count( $groupList ) < 2 )
         {
            return ;
         }
         
         $cursor = $db -> listDomain() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'listDomain错误' ) ;
         $this -> assertNotEmpty( $cursor, 'listDomain错误' ) ;
         $num = 0 ;
         while( $record = $cursor -> next() ) {
            ++$num ;
         }
         $this -> assertEquals( 1, $num, 'listDomain错误' ) ;
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_isStandlone
    * @depends test_getGroupList
    */
   public function test_getDomain( $db, $isStandlone, $groupList )
   {
      if( $isStandlone == false )
      {
         if( count( $groupList ) < 2 )
         {
            return ;
         }

         $domainObj = $db -> getDomain( 'php_driver_my_domain' ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getDomain错误' ) ;
         $this -> assertNotEmpty( $domainObj, 'getDomain错误' ) ;
         
         //这个应该是不存在的
         $domainObj = $db -> getDomain( 'php_driver_my_domain1' ) ;
         $err = $db -> getError() ;
         $this -> assertNotEquals( 0, $err['errno'], 'getDomain错误' ) ;
         $this -> assertEmpty( $domainObj, 'getDomain错误' ) ;
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_isStandlone
    * @depends test_getGroupList
    */
   public function test_dropDomain( $db, $isStandlone, $groupList )
   {
      if( $isStandlone == false )
      {
         if( count( $groupList ) < 2 )
         {
            return ;
         }
         
         $err = $db -> dropDomain( 'php_driver_my_domain' ) ;
         $this -> assertEquals( 0, $err['errno'], 'dropDomain错误' ) ;
      }
   }
}

?>
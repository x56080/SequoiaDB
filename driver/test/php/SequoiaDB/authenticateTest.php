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

   Source File Name = authenticateTest.php

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
class SequoiaDB_Auth_Test extends PHPUnit_Framework_TestCase
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
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
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
    * @depends test_isStandlone
    */
   public function test_createUser( $isStandlone )
   {
      if( $isStandlone == false )
      {
         $db = new SequoiaDB();
         $err = $db -> connect( $this->address ) ;
         $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->address.' )' ) ;
   
         $err = $db -> createUser( 'mike', '123456789' ) ;
         $this -> assertEquals( 0, $err['errno'], 'createUser错误' ) ;
         
         $err = $db -> connect( $this->address ) ;
         $this -> assertEquals( -179, $err['errno'], 'createUser错误' ) ;
      }
   }
   
   /**
    * @depends test_isStandlone
    */
   public function test_removeUser( $isStandlone )
   {
      if( $isStandlone == false )
      {
         $db = new SequoiaDB();
         $err = $db -> connect( $this->address, 'mike', '123456789' ) ;
         $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->address.' )' ) ;
   
         $err = $db -> removeUser( 'mike', '123456789' ) ;
         $this -> assertEquals( 0, $err['errno'], 'createUser错误' ) ;
         
         $err = $db -> connect( $this->address ) ;
         $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 数组参数: '.$this->address.' )' ) ;
      }
   }
}

?>
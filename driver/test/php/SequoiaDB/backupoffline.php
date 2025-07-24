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

   Source File Name = backupoffline.php

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
class SequoiaDB_BackupOffline_Test extends PHPUnit_Framework_TestCase
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
   public function test_backupOffline( $db )
   {
      $err = $db -> backupOffline( array( 'Name' => 'myBackup_1' ) ) ;
      $this -> assertEquals( 0, $err['errno'], 'backupOffline错误' ) ;
   }
   
   /**
    * @depends test_connect
    */
   public function test_listProcedure( $db )
   {
      $cursor = $db -> listBackup( array( 'Name' => 'myBackup_1' ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'listBackup错误' ) ;
      $this -> assertNotEmpty( $cursor, 'listBackup错误' ) ;
      $num = 0 ;
      while( $record = $cursor -> next() ) {
         ++$num ;
      }
      $this -> assertNotEquals( 0, $num, 'listBackup错误' ) ;
   }
     
   /**
    * @depends test_connect
    */
   public function test_removeBackup( $db )
   {
      $err = $db -> removeBackup( array( 'Name' => 'myBackup_1' ) ) ;
      $this -> assertEquals( 0, $err['errno'], 'removeBackup错误' ) ;
      
      $cursor = $db -> listBackup( array( 'Name' => 'myBackup_1' ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'listBackup错误' ) ;
      $this -> assertNotEmpty( $cursor, 'listBackup错误' ) ;
      $num = 0 ;
      while( $record = $cursor -> next() ) {
         ++$num ;
      }
      $this -> assertEquals( 0, $num, 'removeBackup错误' ) ;
   }
}

?>
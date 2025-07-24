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

   Source File Name = sessionTest.php

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
class SequoiaDB_Session_Test extends PHPUnit_Framework_TestCase
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
   public function test_setSessionAttr( $db )
   {
      $err = $db -> setSessionAttr( array( 'PreferedInstance' => 'm' ) ) ;
      $this -> assertEquals( 0, $err['errno'], 'setSessionAttr错误' ) ;
   }
   
   /**
    * @depends test_connect
    */
   public function test_forceSession( $db )
   {
      $sessionID = -1 ;
      $cursor = $db -> list( SDB_LIST_SESSIONS_CURRENT ) ;
      while( $record = $cursor -> next() ) {
         $sessionID = $record['SessionID'] ;
      }
      if( $sessionID > 0 )
      {
         //有session
         $err = $db -> forceSession( $sessionID ) ;
         if( $err['errno'] == -16 )
         {
            //把自己断开连接了, 说明forceSession正常
            return ;
         }
         $this -> assertEquals( 0, $err['errno'], 'forceSession错误' ) ;
      }
      
      $cursor = $db -> execSQL( 'select * from $SNAPSHOT_SESSION where Status="Running"' ) ;
      $sessionID = -1 ;
      $nodename = '' ;
      while( $record = $cursor -> next() )
      {
         if( $record['Type'] == 'ShardAgent' )
         {
            $nodename  = $record['NodeName'] ;
            $sessionID = $record['SessionID'] ;
            break ;
         }
      }
      if( $sessionID > 0 )
      {
         //有session
         $hostname = explode( ':', $nodename ) ;
         $svcname  = $hostname[1] ;
         $hostname = $hostname[0] ;
         $err = $db -> forceSession( $sessionID, array( 'HostName' => $hostname, 'svcname' => $svcname ) ) ;
         if( $err['errno'] == -16 )
         {
            //把自己断开连接了, 说明forceSession正常
            return ;
         }
         $this -> assertEquals( 0, $err['errno'], 'forceSession错误' ) ;
      }
   }
}

?>
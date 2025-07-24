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

   Source File Name = nodeTest.php

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
class Group_node_Test extends PHPUnit_Framework_TestCase
{
   protected $hostname ;
   protected $port ;
   protected $address ;

   protected function setUp()
   {
      $this -> hostname = empty( $_POST['hostname'] ) ? '127.0.0.1' : $_POST['hostname'] ;
      $this -> port = empty( $_POST['port'] ) ? '11810' : $_POST['port'] ;
      $this -> address = $this -> hostname.':'.$this -> port ;
      $this -> isLocal = empty( $_POST['islocal'] ) ? false : $_POST['islocal'] ;
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
    * @depends test_connect
    * @depends test_isStandlone
    */
   public function test_getGroup( $db, $isStandlone )
   {
      $group = null ;
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
         if( count( $groupList ) <= 0 )
         {
            return ;
         }
         $groupName = $groupList[0] ;
         $group = $db -> getGroup( $groupName ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getGroup错误' ) ;
         $this -> assertNotEmpty( $group, 'getGroup错误' ) ;
      }
      return $group ;
   }
    
   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_getDetail( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $detail = $group -> getDetail() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getDetail错误' ) ;
         $this -> assertEquals( 0, $detail['Role'], 'getDetail错误' ) ;
      }
   }

   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_getMaster( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $nodeObj = $group -> getMaster() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getMaster错误' ) ;
         $this -> assertNotEmpty( $nodeObj, 'getMaster错误' ) ;
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_getSlave( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $nodeObj = $group -> getSlave() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getSlave错误' ) ;
         $this -> assertNotEmpty( $nodeObj, 'getSlave错误' ) ;
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_getNode( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $detail = $group -> getDetail() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getDetail错误' ) ;
         $this -> assertEquals( 0, $detail['Role'], 'getDetail错误' ) ;
         
         if( count( $detail['Group'] ) <= 0 )
         {
            return ;
         }
         
         $nodeName = $detail['Group'][0]['HostName'].':'.$detail['Group'][0]['Service'][0]['Name'] ;
         
         //C驱动有bug,等改为再测试
         return ;
         $nodeObj = $group -> getNode( $nodeName ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getNode错误' ) ;
         $this -> assertNotEmpty( $nodeObj, 'getNode错误' ) ;
      }
   }

   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_createNode_removeNode( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $isFind = false ;
         $groupName = '' ;
         $hostName = '' ;
         $serviceName = '22200' ;
         $path = '' ;
         $cursor = $db -> list( SDB_LIST_GROUPS, '{ $and: [ { GroupName:{ $ne: "SYSCatalogGroup" } }, { GroupName: { $ne: "SYSCoord" } } ] }' ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '获取group列表错误' ) ;
         $this -> assertNotEmpty( $cursor, '获取group列表错误' ) ;
         while( $record = $cursor -> next() )
         {
            if( count( $record['Group'] ) < 7 )
            {
               $groupName = $record['GroupName'] ;
               $hostName = $record['Group'][0]['HostName'] ;
               $path = './database/data/'.$serviceName ;
               $isFind = true ;
               break ;
            }
         }
         if( $isFind == true )
         {
            $group = $db -> getGroup( $groupName ) ;
            $err = $db -> getError() ;
            $this -> assertEquals( 0, $err['errno'], 'getGroup错误' ) ;
            $this -> assertNotEmpty( $group, 'getGroup错误' ) ;
            
            $err = $group -> createNode( $hostName, $serviceName, $path ) ;
            $this -> assertEquals( 0, $err['errno'], 'createNode错误' ) ;
            
            $err = $group -> removeNode( $hostName, $serviceName ) ;
            $this -> assertEquals( 0, $err['errno'], 'removeNode错误' ) ;
         }
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_attachNode_detachNode( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $isFind = false ;
         $groupName = '' ;
         $hostName = '' ;
         $serviceName = '' ;
         $cursor = $db -> list( SDB_LIST_GROUPS, '{ $and: [ { GroupName:{ $ne: "SYSCatalogGroup" } }, { GroupName: { $ne: "SYSCoord" } } ] }' ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '获取group列表错误' ) ;
         $this -> assertNotEmpty( $cursor, '获取group列表错误' ) ;
         while( $record = $cursor -> next() )
         {
            if( count( $record['Group'] ) > 1 )
            {
               $groupName = $record['GroupName'] ;
               $primary = $record['PrimaryNode'] ;
               
               foreach( $record['Group'] as $key => $value )
               {
                  if( $value['NodeID'] != $primary )
                  {
                     $hostName = $record['Group'][$key]['HostName'] ;
                     $serviceName = $record['Group'][$key]['Service'][0]['Name'] ;
                     $isFind = true ;
                     break ;
                  }
               }
               break ;
            }
         }
         if( $isFind == true )
         {
            $group = $db -> getGroup( $groupName ) ;
            $err = $db -> getError() ;
            $this -> assertEquals( 0, $err['errno'], 'getGroup错误' ) ;
            $this -> assertNotEmpty( $group, 'getGroup错误' ) ;
            
            $err = $group -> detachNode( $hostName, $serviceName ) ;
            $this -> assertEquals( 0, $err['errno'], 'detachNode错误' ) ;
            
            $err = $group -> attachNode( $hostName, $serviceName ) ;
            $this -> assertEquals( 0, $err['errno'], 'attachNode错误' ) ;
         }
      }
   }
}
?>
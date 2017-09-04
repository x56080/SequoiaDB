<?php
class cl_Test extends PHPUnit_Framework_TestCase
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
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
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
      $cl = $cs -> selectCL( 'test_bar' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cl错误' ) ;
      return $cl ;
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
    * @depends test_select_cs
    */
   public function test_drop_cl( $db, $cs )
   {
      $cl = $cs -> selectCL( 'drop_bar' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cl, '创建cl错误' ) ;

      $err = $cl -> drop() ;
      $this -> assertEquals( 0, $err['errno'], '删除cl错误' ) ;

      $cl = $cs -> getCL( 'drop_bar' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( -23, $err['errno'], '获取cl错误' ) ;
      $this -> assertEmpty( $cl, '获取cl错误' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_isStandlone
    */
   public function test_alter( $db, $cs, $isStandlone )
   {
      if( $isStandlone == false )
      {
         $cl = $cs -> createCL( 'alter_bar', array( 'ReplSize' => 1 ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
   
         $err = $cl -> alter( array( 'ReplSize' => -1 ) ) ;
         $this -> assertEquals( 0, $err['errno'], 'alter错误' ) ;
         
         $fullName = $cl -> getFullName() ;
         $cursor = $db -> snapshot( SDB_SNAP_CATALOG, array( 'Name' => $fullName ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '查询快照错误' ) ;
         $this -> assertNotEmpty( $cursor, '查询快照错误' ) ;
         $record = $cursor -> next() ;
         $this -> assertNotEmpty( $record, '查询快照错误' ) ;
         $this -> assertEquals( -1, $record['ReplSize'], 'alter错误' ) ;
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_isStandlone
    */
   public function test_split( $db, $cs, $isStandlone )
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
         
         if( count( $groupList ) < 2 )
         {
            return ;
         }
         
         $cl = $cs -> createCL( 'range_percent', array( 'ReplSize' => -1, 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'range', 'Group' => $groupList[0] ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
         
         $err = $cl -> insert( '{ a : 1 }' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录错误' ) ;
         
         $err = $cl -> split( $groupList[0], $groupList[1], 50 ) ;
         $this -> assertEquals( 0, $err['errno'], 'split错误' ) ;
         
         $name = $cl -> getFullName() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '获取cs.cl名错误' ) ;
         
         $cursor = $db -> snapshot( SDB_SNAP_CATALOG, array( 'Name' => $name ), array( 'CataInfo' => 1 ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '快照查询失败' ) ;
         $this -> assertNotEmpty( $cursor, '快照查询失败' ) ;
         $record = $cursor -> next() ;
         $this -> assertNotEmpty( $record, '快照查询失败' ) ;
         $this -> assertEquals( 2, count( $record['CataInfo'] ), 'split cl错误' ) ;
         $this -> assertTrue( is_object( $record['CataInfo'][0]['LowBound']['a'] ) && is_a( $record['CataInfo'][0]['LowBound']['a'], 'SequoiaMinKey' ), 'split cl错误' ) ;
         $this -> assertTrue( $record['CataInfo'][0]['UpBound']['a'] == 1, 'split cl错误' ) ;
         $this -> assertTrue( $record['CataInfo'][1]['LowBound']['a'] == 1, 'split cl错误' ) ;
         $this -> assertTrue( is_object( $record['CataInfo'][1]['UpBound']['a'] ) && is_a( $record['CataInfo'][1]['UpBound']['a'], 'SequoiaMaxKey' ), 'split cl错误' ) ;
         
         $cl = $cs -> createCL( 'range_start_end', array( 'ReplSize' => -1, 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'range', 'Group' => $groupList[0] ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
         
         $err = $cl -> insert( '{ a : 1 }' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录错误' ) ;
         
         $err = $cl -> split( $groupList[0], $groupList[1], '{a:0}', '{a:10}' ) ;
         $this -> assertEquals( 0, $err['errno'], 'split错误' ) ;
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_isStandlone
    */
   public function test_splitAsync( $db, $cs, $isStandlone )
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
         
         if( count( $groupList ) < 2 )
         {
            return ;
         }
         
         $cl = $cs -> createCL( 'range_percent2', array( 'ReplSize' => -1, 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'range', 'Group' => $groupList[0] ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
         
         $err = $cl -> insert( '{ a : 1 }' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录错误' ) ;
         
         $err = $cl -> splitAsync( $groupList[0], $groupList[1], 50 ) ;
         $this -> assertEquals( 0, $err['errno'], 'split错误' ) ;
         $this -> assertGreaterThanOrEqual( 0, intval($err['taskID'] -> __toString()), 'split错误' ) ;
         
         $cl = $cs -> createCL( 'range_start_end2', array( 'ReplSize' => -1, 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'range', 'Group' => $groupList[0] ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
         
         $err = $cl -> insert( '{ a : 1 }' ) ;
         $this -> assertEquals( 0, $err['errno'], '插入记录错误' ) ;
         
         $err = $cl -> splitAsync( $groupList[0], $groupList[1], '{a:0}', '{a:10}' ) ;
         $this -> assertEquals( 0, $err['errno'], 'split错误' ) ;
         $this -> assertGreaterThanOrEqual( 0, intval($err['taskID'] -> __toString()), 'split错误' ) ;
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_getFullName( $db, $cs, $cl )
   {
      $name = $cl -> getFullName() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cs.cl名字错误' ) ;
      $this -> assertEquals( 'test_foo.test_bar', $name, '获取cs.cl名字错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_getCSName( $db, $cs, $cl )
   {
      $name = $cl -> getCSName() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cs名字错误' ) ;
      $this -> assertEquals( 'test_foo', $name, '获取cs名字错误' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_getName( $db, $cs, $cl )
   {
      $name = $cl -> getName() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cl名字错误' ) ;
      $this -> assertEquals( 'test_bar', $name, '获取cl名字错误' ) ;
      
      $name = $cl -> getCollectionName() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '获取cl名字错误' ) ;
      $this -> assertEquals( 'test_bar', $name, '获取cl名字错误' ) ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_isStandlone
    */
   public function test_attachCL( $db, $cs, $isStandlone )
   {
      if( $isStandlone == false )
      {        
         $cl = $cs -> createCL( 'sub', array( 'ReplSize' => -1 ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
         $sub = $cl ;
         $subFullName = $sub -> getFullName() ;
         
         $cl = $cs -> createCL( 'main', array( 'IsMainCL' => true, 'ShardingKey' => array( 'a' => 1 ), 'ReplSize' => -1 ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
         $this -> assertNotEmpty( $cl, '创建cl错误' ) ;
         $main = $cl ;
         $err = $main -> attachCL( $subFullName, array( 'LowBound' => array( 'a' => 0 ), 'UpBound' => array( 'a' => 100 ) ) ) ;
         $this -> assertEquals( 0, $err['errno'], 'attach cl错误' ) ;
         $mainFullName = $main -> getFullName() ;
         
         $cursor = $db -> snapshot( SDB_SNAP_CATALOG, array( 'Name' => $mainFullName ), array( 'CataInfo' => 1 ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '快照查询失败' ) ;
         $this -> assertNotEmpty( $cursor, '快照查询失败' ) ;
         $record = $cursor -> next() ;
         $this -> assertNotEmpty( $record, '快照查询失败' ) ;
         $this -> assertNotEquals( 0, count( $record['CataInfo'] ), 'attach cl错误' ) ;
         $this -> assertEquals( $subFullName, $record['CataInfo'][0]['SubCLName'], 'attach cl错误' ) ;
         $this -> assertEquals( 0,   $record['CataInfo'][0]['LowBound']['a'], 'attach cl错误' ) ;
         $this -> assertEquals( 100, $record['CataInfo'][0]['UpBound']['a'],  'attach cl错误' ) ;
      }
   }
   
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_isStandlone
    */
   public function test_detachCL( $db, $cs, $isStandlone )
   {
      if( $isStandlone == false )
      {        
         $cl = $cs -> selectCL( 'sub' ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '获取cl错误' ) ;
         $this -> assertNotEmpty( $cl, '获取cl错误' ) ;
         $sub = $cl ;
         $subFullName = $sub -> getFullName() ;
         
         $cl = $cs -> selectCL( 'main' ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '获取cl错误' ) ;
         $this -> assertNotEmpty( $cl, '获取cl错误' ) ;
         $main = $cl ;

         $err = $main -> detachCL( $subFullName ) ;
         $this -> assertEquals( 0, $err['errno'], 'detach cl错误' ) ;
         $mainFullName = $main -> getFullName() ;
         
         $cursor = $db -> snapshot( SDB_SNAP_CATALOG, array( 'Name' => $mainFullName ), array( 'CataInfo' => 1 ) ) ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], '快照查询失败' ) ;
         $this -> assertNotEmpty( $cursor, '快照查询失败' ) ;
         $record = $cursor -> next() ;
         $this -> assertNotEmpty( $record, '快照查询失败' ) ;
         $this -> assertEquals( 0, count( $record['CataInfo'] ), 'detach cl错误' ) ;
      }
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    */
   public function test_clear( $db, $cs )
   {
      $err = array() ;
      for( $i = 0; $i < 20; ++$i )
      {
         $err = $cs -> drop() ;
         if( $err['errno'] == -147 )
         {
            sleep( 1 ) ;
         }
         else
         {
            $this -> assertEquals( 0, $err['errno'], '删除cs错误' ) ;
            break ;
         }
      }
      $this -> assertEquals( 0, $err['errno'], '删除cs错误' ) ;
   }
}
?>

<?php
class Group_Test extends PHPUnit_Framework_TestCase
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
   public function test_isCatalog( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $isCata = $group -> isCatalog() ;
         $this -> assertEquals( false, $isCata, 'isCatalog错误' ) ;
      }
   }

   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_getName( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $name = $group -> getName() ;
         $this -> assertNotEquals( "", $name, 'getName错误' ) ;
      }
   }

   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_start_stop( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $err = $group -> stop() ;
         $this -> assertEquals( 0, $err['errno'], 'stop错误' ) ;
         
         $err = $group -> start() ;
         $this -> assertEquals( 0, $err['errno'], 'start错误' ) ;
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_reelect( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
         $err = $group -> reelect( array( 'Seconds' => 90 ) ) ;
         $this -> assertEquals( 0, $err['errno'], 'reelect错误' ) ;
                 
         $node = $group -> getMaster() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'getMaster错误' ) ;
         $this -> assertNotEmpty( $group, 'getMaster错误' ) ;
         
         $nodeDB = $node -> connect() ;
         $err = $db -> getError() ;
         $this -> assertEquals( 0, $err['errno'], 'connect错误' ) ;
         $this -> assertNotEmpty( $nodeDB, 'connect错误' ) ;
      }
   }
   
   /**
    * @depends test_connect
    * @depends test_getGroup
    * @depends test_isStandlone
    */
   public function test_getNodeNum( $db, $group, $isStandlone )
   {
      if( $group != null && $isStandlone == false )
      {
      }
   }
}
?>
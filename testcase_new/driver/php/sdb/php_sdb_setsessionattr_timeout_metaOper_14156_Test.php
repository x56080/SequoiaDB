/****************************************************
@description:      setSessionAttr
@testlink cases:   seqDB-14156
@modify list:
        2018-1-23 xiaoni huang init
****************************************************/
<?php
define('Cur_Path', dirname(__FILE__));
include_once Cur_Path.'/../commlib/ReplicaGroupMgr.php';
include_once Cur_Path.'/../global.php';
class setSessionAttr1415602 extends PHPUnit_Framework_TestCase
{   
   private static $address;
   private static $db ;
   private static $groupMgr; 
   
   protected static $csDB ;
   protected static $clDB ;
   protected static $csName = 'cs14156_01';
   protected static $clName  = 'cl';
   
   protected static $mclDB ;   
   protected static $mclName = 'mcl';
   protected static $sclName = 'scl';
   
   protected static $csName2 = 'cs14156_02'; 
   protected static $clName2 = 'cl_2';
   
   private static $skipTestCase = false;   

   public static function setUpBeforeClass()
   {
      echo "\n---Begin to init in the setUpBeforeClass.\n"; 
      
      // connect
      self::$address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      self::$db = new SequoiaDB();
      $err = self::$db -> connect( self::$address );
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
      
      // judge groupNum
      self::$groupMgr = new ReplicaGroupMgr(self::$db);
      if ( self::$groupMgr -> getError() != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return;
      } 
      
      if (self::$groupMgr -> getDataGroupNum() < 1)
      {
         self::$skipTestCase = true ;
         return;
      } 
      
      // clean env
      self::$db -> dropCS( self::$csName );
      self::$db -> dropCS( self::$csName2 );      
      
      // create cs
      self::$csDB = self::$db -> selectCS( self::$csName );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".self::$db -> getError()['errno']);
      }
      
      // create cl
      self::$clDB = self::$csDB -> selectCL( self::$clName );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // create mainCL 
      $options = array( 'ShardingKey' => array( 'a' => 1 ), 'IsMainCL' => true );
      self::$mclDB = self::$csDB -> selectCL( self::$mclName, $options );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // create subCL
      $options = array( 'ShardingKey' => array( 'a' => 1 ), 'ShardingType' => 'hash' );
      self::$csDB -> selectCL( self::$sclName, $options );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // insert
      $records = array();
      for ($i = 0; $i < 10; $i++) {
         $recd = array( 'a' => $i, 'b' => "test1111111" );
         $records[$i] = $recd;
      }
      $err = self::$clDB -> bulkInsert( $records );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to in, errno=".self::$db -> getError()['errno']);
      }
   }
   
   public function setUp()
   {
      if( self::$skipTestCase === true )
      {
         $this -> markTestSkipped( "init failed" );
      }
   }

  // metadata operations sometimes do not hit the point
   public function test_createCS()
   {       
      echo "\n---Begin to createCS.\n";
      $tmpDB = new SequoiaDB( self::$address );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      // setSessionAttr
      $err = $tmpDB -> setSessionAttr( array( 'Timeout' => 1 ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      $err = $tmpDB -> createCS( self::$csName2, '{PageSize:4096}' );
      $this -> assertEquals( -13, $err['errno'] ); 
      
      $tmpDB -> close();
   }
   
   public function test_createCL()
   {  
      echo "\n---Begin to createCL.\n";
      $tmpDB = new SequoiaDB( self::$address );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      $cs = $tmpDB -> getCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      // setSessionAttr
      $err = $tmpDB -> setSessionAttr( array( 'Timeout' => 1 ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      $err = $cs -> createCL( self::$clName2 );
      $this -> assertEquals( -13, $err['errno'] );
      
      $tmpDB -> close();
   }
   
   public function test_alterCL()
   {  
      echo "\n---Begin to alterCL.\n";
      $tmpDB = new SequoiaDB( self::$address );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      $cs = $tmpDB -> getCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );       
      $cl = $cs -> getCL( self::$clName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      // setSessionAttr
      $err = $tmpDB -> setSessionAttr( array( 'Timeout' => 1 ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      $err = $cl -> alter( array( 'ReplSize' => -1 ) );
      $this -> assertEquals( -13, $err['errno'] );
      
      $tmpDB -> close();
   }
   
   public function test_attachCL() 
   {
      echo "\n---Begin to attachCL.\n";
      $tmpDB = new SequoiaDB( self::$address );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      $cs = $tmpDB -> getCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );       
      $cl = $cs -> getCL( self::$mclName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      // setSessionAttr
      $err = $tmpDB -> setSessionAttr( array( 'Timeout' => 1 ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      $options = array( 'LowBound' => array( 'a' => 0 ), 'UpBound' => array( 'a' => 100 ) );
      $err = $cl -> attachCL( self::$csName.".".self::$sclName, $options );
      $this -> assertEquals( -13, $err['errno'] );
      
      $tmpDB -> close();
   }
   /*
   public function test_dropCL()
   {  
      echo "\n---Begin to dropCL.\n";
      $tmpDB = new SequoiaDB( self::$address );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      $cs = $tmpDB -> getCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      // setSessionAttr
      $err = $tmpDB -> setSessionAttr( array( 'Timeout' => 1 ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      $err = $cs -> dropCL( self::$clName );
      $this -> assertEquals( -13, $err['errno'] );
      
      $tmpDB -> close();
   }*/
   
   public function test_dropCS()
   {  
      echo "\n---Begin to dropCS.\n";
      $tmpDB = new SequoiaDB( self::$address );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      
      // setSessionAttr
      $err = $tmpDB -> setSessionAttr( array( 'Timeout' => 1 ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      $err = $tmpDB -> dropCS( self::$csName );
      $this -> assertEquals( -13, $err['errno'] );
      
      $tmpDB -> close();
   }
   
   public static function tearDownAfterClass()
   {  
      if ( self::$skipTestCase == false )
      {
         echo "\n---Begin to clean env in the tearDownAfterClass.\n"; 
         
         echo "   Begin to dropCS[".self::$csName."] in the end.\n"; 
         $err = self::$db -> dropCS( self::$csName );  
         if ( $err['errno'] != 0  && $err['errno'] != -34 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
         
         echo "   Begin to dropCS[".self::$csName2."] in the end.\n"; 
         $err = self::$db -> dropCS( self::$csName2 );  
         if ( $err['errno'] != 0  && $err['errno'] != -34 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
      }
      
      self::$db->close();
   }
};
?>
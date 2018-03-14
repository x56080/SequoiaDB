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
class setSessionAttr14156 extends PHPUnit_Framework_TestCase
{ 
   private static $db ;
   private static $groupMgr; 
   private static $groupNames;   
   private static $csDB ;   
   private static $clDB ;
   private static $csName = 'cs14156';
   private static $clName = 'cl';
   private static $skipTestCase = false;  

   public static function setUpBeforeClass()
   {
      echo "\n---Begin to init in the setUpBeforeClass.\n"; 
      
      // connect
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '');
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db, errno=".$err['errno']);
      }
      
      // clean env
      self::$db -> dropCS( self::$csName );
      
      // judge groupNum
      self::$groupMgr = new ReplicaGroupMgr(self::$db);
      if ( self::$groupMgr -> getError() != 0 )
      {
         echo "Failed to connect database, error code: ".$err['errno'] ;
         self::$skipTestCase = true ;
         return;
      } 
      
      if (self::$groupMgr -> getDataGroupNum() < 2)
      {
         self::$skipTestCase = true ;
         return;
      }
      
      // get groupNames      
      self::$groupNames = self::$groupMgr -> getDataGroupNames();  
      
      // create cs
      self::$csDB = self::$db -> selectCS( self::$csName, null );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".self::$db -> getError()['errno']);
      }
      
      // create cl
      $option = array( 'Group' => self::$groupNames[0], 'ReplSize' => 0
               ,'ShardingType' => 'hash', 'ShardingKey' => array( 'a' => 1 ) );
      self::$clDB = self::$csDB -> selectCL( self::$clName, $option );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // insert
      $records = array();
      for ($i = 0; $i < 5000; $i++) 
      {
         array_push( $records, array('a' => $i, 'b' => $i) );
      }
      self::$clDB -> bulkInsert( $records );
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

   public function test_setSessionAttr()
   {  
      echo "\n---Begin to setSessionAttr[set TimeOut=1ms].\n"; 
      $instanceid = 'M';
      $instanceTimeout = 1;
          
      // setSessionAttr
      $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'Timeout' => $instanceTimeout ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      // getSessionAttr
      echo "   Begin to getSessionAttr.\n"; 
      $results = self::$db -> getSessionAttr();
      $this -> assertEquals( 0, self::$db -> getError()['errno'] ); 
      $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
      $this -> assertEquals( (string)$instanceTimeout, $results['Timeout'] );
   }

   public function test_insert()
   {  
      echo "\n---Begin to insert records.\n";
      $records = array();
      for ($i = 0; $i < 3000; $i++) {
         $recd = array( 'a' => $i, 'b' => $i, 't' => "test1111111".$i );
         $records[$i] = $recd;
      }
      $err = self::$clDB -> bulkInsert( $records );
      $this -> assertEquals( -13, $err['errno'] );
   }

   public function test_update()
   {  
      echo "\n---Begin to update records.\n";
      $rule = array( '$inc' => array( 'b' => 100 ) );
      $cond = array( 'a' => array ( '$gte' => 1 ) );
      $err = self::$clDB -> update( $rule, $cond );
      $this -> assertEquals( -13, $err['errno'] );
   }
   
/* may not hit a bit
   public function test_remove()
   {  
      echo "\n---Begin to remove records.\n";
      $cond = array( 't1' => array ( '$gte' => 0 ) );
      $err = self::$clDB -> remove( $cond );
      $this -> assertEquals( -13, $err['errno'] );
   }
*/

   public function test_split()
   {  
      echo "\n---Begin to split records.\n";
      $err = self::$clDB -> split( self::$groupNames[0], self::$groupNames[1], 50 );
      $this -> assertEquals( -13, $err['errno'] );
   }
   
   public static function tearDownAfterClass()
   {  
      if ( self::$skipTestCase == false )
      {
         echo "\n---Begin to clean env in the tearDownAfterClass.\n"; 
         
         echo "   Begin to recover session[set Timeout=10000ms] in the end.\n"; 
         $err = self::$db -> setSessionAttr( array( 'Timeout' => 100000 ) ); 
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
         
         sleep(2);
         
         echo "   Begin to dropCS in the end.\n"; 
         $err = self::$db -> dropCS( self::$csName );  
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
      }
      
      $err = self::$db->close();
   }
};
?>
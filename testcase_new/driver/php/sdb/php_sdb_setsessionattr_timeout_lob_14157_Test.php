/****************************************************
@description:      setSessionAttr
@testlink cases:   seqDB-14157
@modify list:
        2018-1-23 xiaoni huang init
****************************************************/
<?php
define('Cur_Path', dirname(__FILE__));
include_once Cur_Path.'/../commlib/ReplicaGroupMgr.php';
include_once Cur_Path.'/../commlib/lob.php';
include_once Cur_Path.'/../global.php';
class setSessionAttr14157 extends PHPUnit_Framework_TestCase
{
   private static $db ;
   private static $groupMgr; 
   private static $oid ;
   protected static $csDB ;
   protected static $clDB ;
   protected static $csName = 'cs14157';
   protected static $clName = 'cl';
   private static $skipTestCase = false;  
   
   public static function setUpBeforeClass()
   {
      // connect
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '');
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
      
      if (self::$groupMgr -> getGroupNum() < 1)
      {
         self::$skipTestCase = true ;
         return;
      } 
      
      // create cs
      self::$csDB = self::$db -> selectCS( self::$csName, null );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".self::$db -> getError()['errno']);
      }
      
      // create cl
      $option = array( 'ReplSize' => 0 );
      self::$clDB = self::$csDB -> selectCL( self::$clName, $option );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // generate oid
      echo "\n---Begin to generate uniq oid.\n"; 
      self::$oid = uniqid();
      self::$oid = md5(self::$oid);       
      self::$oid=substr(self::$oid, 8); 
      //var_dump(self::$oid);
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
   
   public function test_Lob()
   {
      echo "\n---Begin to open lob.\n"; 
      $lob = new Lob( self::$db, self::$clDB );
      $err = $lob -> open( self::$oid, SDB_LOB_CREATEONLY );
      $this -> assertEquals( -13, $err );
   }
   
   public static function tearDownAfterClass()
   {
      if ( self::$skipTestCase == false )
      {
         echo "\n---Begin to recover session[set Timeout=10000ms] in the end.\n"; 
         $err = self::$db -> setSessionAttr( array( 'Timeout' => 100000 ) ); 
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }      
         
         echo "   Begin to dropCS in the end.\n"; 
         $err = self::$db -> dropCS( self::$csName ); 
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
      }
      
      $err = self::$db->close();
   }
      
}
?>

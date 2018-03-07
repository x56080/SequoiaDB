/****************************************************
@description:      setSessionAttr
@testlink cases:   seqDB-14150/seqDB-14152/seqDB-14153/seqDB-14155
@modify list:
        2018-1-23 xiaoni huang init
****************************************************/
<?php
define('Cur_Path', dirname(__FILE__));
include_once Cur_Path.'/../commlib/ReplicaGroupMgr.php';
include_once Cur_Path.'/../global.php';
class setSessionAttr14150 extends PHPUnit_Framework_TestCase
{
   protected static $db ;
   private static $groupMgr;
   private static $groupName = 'group14150';
   private static $nodeAddrs = array();
   private static $mastHostName;
   
   protected static $clDB ;
   protected static $csName  = 'cs14150';
   protected static $clName  = 'cl';
   
   private static $instIds = array( 6, 8 );
   
   private static $skipTestCase = false;  

   public static function setUpBeforeClass()
   {
      echo "\n---Begin to exec setUpBeforeClass.\n"; 
      echo "   ......\n"; 
      
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
      
      if (self::$groupMgr -> getDataGroupNum() < 2)
      {
         self::$skipTestCase = true ;
         return;
      } 
      
      // add dataGroup
      echo "   Begin to add dataGroup.\n";       
      $group = self::$groupMgr -> addDataGroup( self::$groupName );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to add group, errno=".self::$db -> getError()['errno']);
      }
      
      // add nodes  
      echo "   Begin to add nodes.\n";
      $startPort = globalParameter::getSpareportStart();
      $endPort = globalParameter::getSpareportStop();
      for ($i = 0; $i < 2; $i++ ) {  
         $hosts = self::$groupMgr -> getAllHostNamesOfDeploy();
         $hostName = $hosts[mt_rand(0,count($hosts)-1)] ;
         
         $options = array( 'instanceid' => self::$instIds[$i] );
         $group -> addNode( $hostName, $startPort, globalParameter::getDbPathPrefix()."/".$startPort, $options );
         $errno = self::$db -> getError()['errno'];
         if ( $errno == -145 && ( $startPort + 3 ) <= $endPort ) {
            $startPort += 10;
         }
         else if ( $errno != 0 && $errno != -145 )
         {
            throw new Exception("failed to add node, errno=".self::$db -> getError()['errno']);
         }
         
         $addr = $hostName.':'.$startPort;
         array_push( self::$nodeAddrs,  $addr );
         
         $startPort += 10;
      }
      //var_dump( self::$nodeAddrs );
      
      // start group 
      echo "   Begin to start nodes.\n"; 
      $group -> start();
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to start group, errno=".self::$db -> getError()['errno']);
      }
      
      // get master nodeAddr
      echo "   Begin to waiting vote master node.\n";
      $rg = self::$db -> getGroup( self::$groupName );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to start group, errno=".self::$db -> getError()['errno']);
      }
      
      $maxtimes = 30;
      $currTimes = 0;
      for ($i = 0; $i < $maxtimes; $i++ ) 
      {
         $mstNode = $rg -> getMaster();
         if ( self::$db -> getError()['errno'] == 0 )
         {
            break;
         } elseif ( self::$db -> getError()['errno'] != 0 && $currTimes < $maxtimes )
         {
            sleep( 2 );
            $currTimes++;
         } else {
            throw new Exception("failed to start group, errno=".self::$db -> getError()['errno']);
         }
      }
      
      self::$mastHostName = $mstNode -> getName();
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to start group, errno=".self::$db -> getError()['errno']);
      }
      
      echo "   ......\n"; 
      
      // create cs
      $csDB = self::$db -> selectCS( self::$csName, null );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cs, errno=".self::$db -> getError()['errno']);
      }
      
      // create cl
      $option = array( 'Group' => self::$groupName, 'ReplSize' => 0 );
      self::$clDB = $csDB -> selectCL( self::$clName, $option );
      if ( self::$db -> getError()['errno'] != 0 )
      {
         throw new Exception("failed to create cl, errno=".self::$db -> getError()['errno']);
      }
      
      // insert
      self::$clDB -> insert('{a:1}');
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

   // seqDB-14150
   public function test_setSessionAttrForNum()
   {  
      echo "\n---Begin to setSessionAttr[num].\n"; 
      $expNodeName = self::$nodeAddrs[0];
      $instanceid  = self::$instIds[0];
          
      // setSessionAttr
      $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      // getSessionAttr
      echo "   Begin to getSessionAttr.\n"; 
      $results = self::$db -> getSessionAttr();
      $this -> assertEquals( 0, $err['errno'] ); 
      $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
      $this -> assertEquals( 'random', $results['PreferedInstanceMode'] );
      $this -> assertEquals( '-1', $results['Timeout'] );
      
      // explain
      echo "   Begin to exec explain.\n"; 
      $record = self::$clDB -> explain('{a:1}') -> current();
      $this -> assertEquals( $expNodeName, $record['NodeName'] );
   }

   // seqDB-14152
   public function test_setSessionAttrForString()
   {  
      echo "\n---Begin to setSessionAttr[string].\n"; 
      $expNodeName = self::$mastHostName;
      $instanceid = 'M';
      $instanceMode = 'random';
      $instanceTimeout = 10000;
      
      // setSessionAttr
      $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'PreferedInstanceMode' => $instanceMode
            , 'Timeout' => $instanceTimeout ) );
      $this -> assertEquals( 0, $err['errno'] );
      
      // getSessionAttr
      echo "   Begin to getSessionAttr.\n"; 
      $results = self::$db -> getSessionAttr();
      $this -> assertEquals( 0, $err['errno'] ); 
      $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
      $this -> assertEquals( $instanceMode, $results['PreferedInstanceMode'] );
      $this -> assertEquals( (string)$instanceTimeout, $results['Timeout'] );
      
      // explain
      echo "   Begin to exec explain.\n"; 
      $record = self::$clDB -> explain('{a:1}') -> current();
      $this -> assertEquals( $expNodeName, $record['NodeName'] );
   }

   // seqDB-14153
   public function test_setSessionAttrForArrayRandom()
   {  
      echo "\n---Begin to setSessionAttr[array, random].\n";
      $expNodeName = self::$nodeAddrs;
      $instanceid = self::$instIds;
      $instanceMode = 'random';
      
      echo "   Begin to exec setSessionAttr -> getSessionAttr -> explain.\n"; 
      $nodeNameArr = array();
      for ($i = 0; $i < 15; $i++) {
         // setSessionAttr
         $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid ) );
         $this -> assertEquals( 0, $err['errno'] );
         
         // getSessionAttr
         $results = self::$db -> getSessionAttr();
         $this -> assertEquals( 0, $err['errno'] ); 
         $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
         $this -> assertEquals( $instanceMode, $results['PreferedInstanceMode'] );
      
         // explain
         $record = self::$clDB -> explain('{a:1}') -> current();
         $nodeNameArr[$i] = $record['NodeName'];
      }
      $this -> assertContains($expNodeName[0], $nodeNameArr);
      $this -> assertContains($expNodeName[1], $nodeNameArr);
   }

   // seqDB-14153
   public function test_setSessionAttrForArrayOrdered()
   {  
      echo "\n---Begin to setSessionAttr[array, ordered].\n";
      $expNodeName = self::$nodeAddrs[0];
      $instanceid = self::$instIds;
      $instanceMode = 'ordered';
      
      echo "   Begin to exec setSessionAttr -> getSessionAttr -> explain.\n"; 
      for ($i = 0; $i < 10; $i++) {          
         // setSessionAttr
         $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'PreferedInstanceMode' => $instanceMode ) );
         $this -> assertEquals( 0, $err['errno'] );
         
         // getSessionAttr
         $results = self::$db -> getSessionAttr();
         $this -> assertEquals( 0, $err['errno'] ); 
         $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
         $this -> assertEquals( $instanceMode, $results['PreferedInstanceMode'] );
         
         // explain
         $record = self::$clDB -> explain('{a:1}') -> current();
         $this -> assertEquals($expNodeName, $record['NodeName']);
      }
   }

   // seqDB-14155
   public function test_setSessionAttrForArrayMix1()
   {  
      echo "\n---Begin to setSessionAttr[array, num and string[M], ordered].\n"; 
      $expNodeName = self::$mastHostName;
      $instanceid = self::$instIds;
      array_push( $instanceid, 'M');
      $instanceMode = 'ordered';
      
      echo "   Begin to exec setSessionAttr -> getSessionAttr -> explain.\n"; 
      for ($i = 0; $i < 10; $i++) 
      {
         // setSessionAttr
         $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'PreferedInstanceMode' => $instanceMode ) );
         $this -> assertEquals( 0, $err['errno'] );
         
         // getSessionAttr
         $results = self::$db -> getSessionAttr();
         $this -> assertEquals( 0, $err['errno'] ); 
         $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
         $this -> assertEquals( $instanceMode, $results['PreferedInstanceMode'] );
         
         // explain
         $record = self::$clDB -> explain('{a:1}') -> current();
         $this -> assertEquals($expNodeName, $record['NodeName']);
      }
   }

   // seqDB-14155
   public function test_setSessionAttrForArrayMix2()
   {  
      echo "\n---Begin to setSessionAttr[array, num and string[m], ordered].\n"; 
      $expNodeName = self::$nodeAddrs[0];
      $instanceMode = 'ordered';
      $instanceid = self::$instIds;
      array_push( $instanceid, 'm');
      
      echo "   Begin to exec setSessionAttr -> getSessionAttr -> explain.\n"; 
      for ($i = 0; $i < 10; $i++) {          
         // setSessionAttr
         $err = self::$db -> setSessionAttr( array( 'PreferedInstance' => $instanceid, 'PreferedInstanceMode' => $instanceMode ) );
         $this -> assertEquals( 0, $err['errno'] );
         
         // getSessionAttr
         $results = self::$db -> getSessionAttr();
         $this -> assertEquals( 0, $err['errno'] ); 
         $this -> assertEquals( $instanceid, $results['PreferedInstance'] );
         $this -> assertEquals( $instanceMode, $results['PreferedInstanceMode'] );
         
         // explain
         $record = self::$clDB -> explain('{a:1}') -> current();
         $this -> assertEquals($expNodeName, $record['NodeName']);
      }
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---Begin to exec tearDownAfterClass.\n"; 
      
      if ( self::$skipTestCase == false )
      {
         echo "   Begin to dropCS.\n";
         $err = self::$db -> dropCS( self::$csName ); 
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to drop cs, errno=".$err['errno']);
         }
         
         echo "   Begin to removeGroup.\n";
         $err = self::$db -> removeGroup( self::$groupName );
         if ( $err['errno'] != 0 )
         {
            throw new Exception("failed to remove group, errno=".self::$db -> getError()['errno']);
         }
      }
      
      $err = self::$db -> close();
   }
};
?>
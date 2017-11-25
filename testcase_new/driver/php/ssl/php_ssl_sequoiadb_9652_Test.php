/****************************************************
@description:      ssl, sequoiadb
@testlink cases:   seqDB-9652
@modify list:
        2017-11-21 xiaoni huang init
****************************************************/
<?php
define('Cur_Path', dirname(__FILE__));
include_once Cur_Path.'/../global.php';
class sslTest965202 extends PHPUnit_Framework_TestCase
{
   protected static $db ;
   protected static $csName  = 'cs9652_seq';
   protected static $clName  = 'cl';

   public static function setUpBeforeClass()
   {
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      self::$db = new Sequoiadb();
      $err = self::$db -> connect($address, '', '', true);
      if ( $err['errno'] != 0 )
      {
         throw new Exception("failed to connect db");
      }
   }

   public function test_ssl()
   {
      // create cs
      self::$db -> createCS( self::$csName, null );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      // get cs
      $csDB = self::$db -> getCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      // create cl
      $csDB -> createCL( self::$clName, null );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );
      
      // drop cs
      self::$db -> dropCS( self::$csName );
      $this -> assertEquals( 0, self::$db -> getError()['errno'] );      
   }
   
   public static function tearDownAfterClass()
   {
      $err = self::$db->close();
   }
};
?>
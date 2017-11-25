/****************************************************
@description:      ssl, securesdb
@testlink cases:   seqDB-9653
@modify list:
        2017-11-21 xiaoni huang init
****************************************************/
<?php
define('Cur_Path', dirname(__FILE__));
include_once Cur_Path.'/../global.php';
class sslTest965301 extends PHPUnit_Framework_TestCase
{
   protected static $db;

   public function test_ssl()
   {
      $address = globalParameter::getHostName().':'.globalParameter::getCoordPort();
      /*
      self::$db = new SecureSdb();
      $err = self::$db -> connect($address, '', '');
      if ( $err['errno'] != -15 )
      {
         throw new Exception("failed to connect db");
      }
      */
   }
   
};
?>
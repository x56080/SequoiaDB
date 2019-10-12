/****************************************************
@description:      check sdb.list() option
@testlink cases:   seqDB-20008
@modify list:
        2019-10-11 Luweikang init
****************************************************/
<?php

include_once dirname(__FILE__).'/../global.php';

class List20008 extends PHPUnit_Framework_TestCase
{
   private static $db;
   
   public static function setUpBeforeClass()
   {
	   self::$db = new Sequoiadb();
           self::$db -> connect(globalParameter::getHostName().':'.
		                globalParameter::getCoordPort()) ;
      self::checkErrno( 0, self::$db -> getError()['errno'] );                     
   }
   
   function test()
   {
      echo "\n---Begin to check list.\n";

      $userArr = array( "admin1", "admin2", "admin3", "admin4", "admin5");

      foreach( $userArr as $i => $user )
      {
         self::$db -> createUser( $user, $user );
      }

      //counts the number of nodes in the current cluster
      $skipNum = count( $userArr ) - 1;
      $returnNum = 1;
      $listCur = self::$db -> list( SDB_LIST_USERS, null, null, null, array("" => "test"), $skipNum, -1 );
      if( empty( $listCur ) ) 
      {
	 foreach( $userArr as $i => $user )
         {
            self::$db -> removeUser( $user, $user );
         }
         throw new Exception('get list SDB_LIST_USERS error, is empty');
      }
      $times = 0;
      while( $record = $listCur -> next())
      {
         $times++;
      }
      if( $times != $returnNum)
      {
         foreach( $userArr as $i => $user )
	 {
	    self::$db -> removeUser( $user, $user );
	 }
	 throw new Exception( 'check list record num error, exp: '. $returnNum .', act: ' . $times );
      }

      $listCur = self::$db -> list( SDB_LIST_USERS, null, null, null, array("" => "test"), 0, $returnNum );
      if( empty( $listCur ) )
      {
	 foreach( $userArr as $i => $user )
	 {
	    self::$db -> removeUser( $user, $user );
         }
         throw new Exception('get list SDB_LIST_USERS error, is empty');
      }
      $times = 0;
      while( $record = $listCur -> next())
      {
         $times++;
      }
      if( $times != $returnNum)
      {
	 foreach( $userArr as $i => $user )
         {
	    self::$db -> removeUser( $user, $user );
	 }
         throw new Exception( 'check list record num error, exp: '. $returnNum .', act: ' . $times );
      }
   }
   
   public static function tearDownAfterClass()
   {
      echo "\n---check list complete.\n";
      $userArr = array( "admin1", "admin2", "admin3", "admin4", "admin5");
      foreach( $userArr as $i => $user )
      {
         self::$db -> removeUser( $user, $user );
      }
      self::$db->close();
   }
   
   private static function checkErrno( $expErrno, $actErrno, $msg = '' )
   {
      if( $expErrno != $actErrno ) 
      {
         throw new Exception( 'expect ['.$expErrno.'] but found ['.$actErrno.']. '.$msg );
      }
   }
   
}
?>

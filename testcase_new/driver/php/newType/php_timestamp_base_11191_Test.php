/****************************************************
@description:      Test SequoiaTimestamp, date range is valid
@testlink cases:   seqDB-11191
@input:        crud, data type: date
@output:     success
@modify list:
        2017-11-30 XiaoNi Huang init
****************************************************/
<?php
define('Cur_Path', dirname(__FILE__));
include_once Cur_Path.'/../func.php';

class DateType11191 extends BaseOperator 
{  
   public function __construct()
   {
      parent::__construct();
   }
   
   function getErrno()
   {
      $this -> err = $this -> db -> getError();
      return $this -> err['errno'];
   }
   
   function createCL( $csName, $clName )
   {
      $options = null;
      return $this -> commCreateCL( $csName, $clName, $options, true );
   }
   
   function insertRecs( $clDB )
   {
     // var_dump( new SequoiaTimestamp( "-2147483648" ) -> __toString() );
     // var_dump( new SequoiaTimestamp( "2147483647" ) -> __toString() );
      
      $recsArray = array( 
         //array( 'a' => 0,  'b' => new SequoiaTimestamp() ),  //nonsuport
         array( 'a' => 1,  'b' => new SequoiaTimestamp( "1902-01-01-00:00:00.000000" ) ), 
         array( 'a' => 2,  'b' => new SequoiaTimestamp( "2037-12-31-23:59:59.999999" ) ), 
         array( 'a' => 3,  'b' => new SequoiaTimestamp( "1902-01-01T00:00:00.000Z" ) ), 
         array( 'a' => 4,  'b' => new SequoiaTimestamp( "2037-12-31T23:59:59.999Z" ) ), 
         array( 'a' => 5,  'b' => new SequoiaTimestamp( "1902-01-01T00:00:00.000+0800" ) ), 
         array( 'a' => 6,  'b' => new SequoiaTimestamp( "2038-01-01T10:00:00.000+0800" ) )
         //array( 'a' => 7,  'b' => new SequoiaTimestamp( "-2147483648" ) ), 
         //array( 'a' => 8,  'b' => new SequoiaTimestamp( "2147483647" ) )
      );
      
      for( $i = 0; $i < count( $recsArray ); $i++ )
      {
         $clDB -> insert( $recsArray[$i] );
      }
   }
   
   function findRecs( $clDB )
   {
      $findRecsArray = array() ;
      $cursor = $clDB -> find( '{b:{$type:1, $et:17}}', '{_id:{$include:0}}', '{a:1}' );   
      while( $record = $cursor -> next() )
      {
         array_push( $findRecsArray, $record );
      }
      
      //var_dump( $findRecsArray );
      return $findRecsArray;
   }
   
   function dropCL( $csName, $clName,$ignoreNotExist )
   {
      $this -> commDropCL( $csName, $clName, $ignoreNotExist );
   }
   
}

class TestDate11191 extends PHPUnit_Framework_TestCase
{
   protected static $dbh;
   private static $csName;
   private static $clName;
   private static $clDB;
   
   public static function setUpBeforeClass()
   {
      self::$dbh = new DateType11191();
      
      echo "\n---Begin to ready parameter.\n";
      self::$csName = self::$dbh -> COMMCSNAME;
      self::$clName = self::$dbh -> COMMCLNAME;
      
      echo "\n---Begin to drop cl in the begin.\n";
      self::$dbh -> dropCL( self::$csName, self::$clName, true );
      
      echo "\n---Begin to create cl.\n";
      self::$clDB = self::$dbh -> createCL( self::$csName, self::$clName );
   }
   
   function test_insert()
   {
      echo "\n---Begin to insert records.\n";
      
      self::$dbh -> insertRecs( self::$clDB );
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( 0, $errno );
   }
   
   function test_find()
   {
      echo "\n---Begin to find records.\n";
      
      $actRecsArray = self::$dbh -> findRecs( self::$clDB );
      
      $errno = self::$dbh -> getErrno();
      $this -> assertEquals( -29, $errno );
      
      $this -> assertCount( 6, $actRecsArray );
      
      $expRecsArray = array( 
         //array( 'a' => 0,  'b' => new SequoiaTimestamp() ),  //nonsuport
         array( 'a' => 1,  'b' => "1902-01-01-00.00.00.000000" ), 
         array( 'a' => 2,  'b' => "2037-12-31-23.00.00.000000" ), 
         array( 'a' => 3,  'b' => "1902-01-01-08.05.52.000000" ), 
         array( 'a' => 4,  'b' => "2038-01-01-07.59.59.999000" ), 
         array( 'a' => 5,  'b' => "1902-01-01-00.05.52.000000" ), 
         array( 'a' => 6,  'b' => "2037-12-31-23.59.59.999000" )
         //array( 'a' => 7,  'b' => "1905-05-06-03.20.00.000000" ) 
         //array( 'a' => 8,  'b' => "1904-05-06-03.20.00.000000" )
      );
      
      for ($i = 0; $i < count( $expRecsArray ); $i++ )
      {
         if ( $i < 2 ) {
            $this -> assertEquals( $expRecsArray[$i]['b'], $actRecsArray[$i]['b'] -> __toString() );
         } else if ($i == 2 || $i == 4 ){
            $this -> assertContains( "1902-01-01", $actRecsArray[$i]['b'] -> __toString(), '$i = '.$i );
         } else if ($i == 3 || $i == 5){
            $this -> assertContains( "2038-01-01", $actRecsArray[$i]['b'] -> __toString(), '$i = '.$i );
         } 
      }
   }
  
}
?>
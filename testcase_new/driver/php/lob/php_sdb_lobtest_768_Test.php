/****************************************************
@description:      lob operate, base case
@testlink cases:   seqDB-7681-7689
@modify list:
        2016-4-27 wenjing wang init
****************************************************/
<?php
   define('Cur_Path', dirname(__FILE__));
   include_once Cur_Path.'/lib/lob.php';
   include_once Cur_Path.'/../global.php';
   class LobTest extends PHPUnit_Framework_TestCase
   {
      private static $db ;
      private static $cs ;
      private static $cl ;
      private static $wmd5 ;
      private static $skipTestCase = false ;
      public static function setUpBeforeClass()
      {
         self::$db = new SequoiaDB();
         $err = self::$db->connect( globalParameter::getHostName() , 
                                    globalParameter::getCoordPort() ) ;
         if ( $err['errno'] != 0 )
         {
            echo "Failed to connect database, error code: ".$err['errno'] ;
            self::$skipTestCase = true ;
            return ;
         }   
         $err = self::$db->setSessionAttr(array('PreferedInstance' => 'm' )) ;
         if ( $err['errno'] != 0 )
         {
            echo "Failed to call setSessionAttr, error code: ".$err['errno'] ;
            self::$skipTestCase = true ;
            return ;
         }
         $random = mt_rand( 1000, 10000 ); 
         $csName = globalParameter::getChangedPrefix().$random;
         $clName = globalParameter::getChangedPrefix().$random;
         $err=self::$db->createCS( $csName );
         if ( $err['errno'] != 0 )
         {
            echo "Failed to connect database, error code: ".$err['errno'] ;
            self::$skipTestCase = true ;
            return ;
         }
         self::$cs = self::$db->selectCS( $csName );
         $err = self::$db->getError();
         if( $err['errno'] != 0 ) 
         {
            echo "Failed to call selectCS, error code: ".$err['errno'] ;
            self::$skipTestCase = true ;
            return;
         }    
         self::$cl = self::$cs->selectCL( $clName );
         $err = self::$db->getError() ;
         if( $err['errno'] != 0 ) {
            echo "Failed to create collection, error code: ".$err['errno'] ;
            self::$skipTestCase = true ;
            return ;
         }
      }
      
      public function setUp()
      {
         if ( self::$skipTestCase == true )
         {
            $this->markTestSkipped( 'init failed' );
         }
      }
      
      private function getOid()
      {
         $oid = uniqid();
         /*if (mt_rand(0,1) == 1)
         {*/
            $oid = md5($oid);
         //}
          
         $oid=substr($oid, 8); 
         //$oid = "573d8ed3573fd85b6e000000";
         return $oid;
      }
      
      private function checkLobExist( $oid, $lob )
      {
         $ret = false;
         $cursor = self::$cl->listLob() ;
         $this->assertEquals( 0, self::$db->getError()['errno'] ) ;
         while( $record = $cursor -> next() )
         {
           // date_default_timezone_set("UTC");
           // $curtime = gettimeofday();
           // $curusec = $curtime['sec'] * 1000000 + $curtime['usec']; 
            if ($record['Oid'] == $oid){
               $ret = true;
            }
         }
         
         $this->assertEquals( -29, self::$db->getError()['errno'] ) ;
         if (!$ret) return $ret;
         
         $ret = false;
         $cr = self::$cl->listLobPieces();
         $this->assertEquals( 0, self::$db->getError()['errno'] ) ;
         while( $record = $cr -> next() )
         {
            if ( $record['Oid'] == $oid ){
               $ret = true;
            }
         }
         
         $this->assertEquals( -29, self::$db->getError()['errno'] ) ;
         return $ret;
      }
      
      public function testLob7681And7688()
      {
         $lob = new Lob( self::$db, self::$cl );
         $oid = $this->getOid();
         $err = $lob->open( $oid, SDB_LOB_CREATEONLY );
         $this->assertEquals( 0, $err );
         
         $err = $lob->write( 1024 );
         $this->assertEquals( 0, $err ) ;
         
         
         //var_dump("write successfully"); 
         $err = $lob->closeLob();
         $this->assertEquals( 0, $err ) ;
         //var_dump("close successfully");
         

         $ret = $this->checkLobExist( $oid, $lob );
         $this->assertEquals( true, $ret ) ;
         $err = $lob->remove( $oid ) ;
         $this->assertEquals( 0, $err ) ;
         $ret = $this->checkLobExist( $oid, $lob );
         $this->assertEquals( false, $ret ) ;
      }
      
      public function ntestWrite7682()
      {
         var_dump("testWriteForNotOpen");
         $lob = new Lob( self::$db, self::$cl );
         $ret = SDB_CLT_INVALID_HANDLE;
         //$err = $lob->write(1024);
         $this->assertEquals( SDB_CLT_INVALID_HANDLE, $ret ) ;
      }
      
      public function testWrite7683()
      {
         $lob = new Lob( self::$db, self::$cl );
         $oid = $this->getOid();
         $err = $lob->open($oid, SDB_LOB_CREATEONLY );
         $this->assertEquals( 0, $err ) ;
         
         $err = $lob->write( 1024 );
         $this->assertEquals( 0, $err ) ;
         
         self::$wmd5 = md5( $lob->getWContent() ); 
         $err = $lob->closeLob();
         $this->assertEquals( 0, $err ) ;

         return $oid;
      }
      
      /**
       * @depends testWrite7683 
       * 
       */
      public function testRead7684And7689($oid)
      {
         $lob = new Lob( self::$db, self::$cl );
         $err = $lob->open( $oid, SDB_LOB_READ );
         $this->assertEquals( 0, $err ) ;
         $err = $lob->read();
         $this->assertEquals( 0, $err ) ;
         
         $ret = $lob->getCreateTime();         
         $this->assertEquals( $ret > new SequoiaINT64 (0), true ) ;
         $err = $lob->closeLob();
         $this->assertEquals( 0, $err ) ;
        
         $this->rbuf = $lob->getRContent() ; 
         $ret =  ( self::$wmd5 == md5( $this->rbuf ) ) ;
         $this->assertEquals( true, $ret ) ;
      }
      
      public function ntestRead7685()
      {
         var_dump( "testReadForNotOpen" ) ;
         $lob = new Lob( self::$db, self::$cl ) ;
         $ret = SDB_CLT_INVALID_HANDLE ;
         //$err = $lob->read();
         $this->assertEquals( SDB_CLT_INVALID_HANDLE, $ret ) ;
      }
      
      public function ntestSeek7686()
      {
         var_dump( "testSeekForWrite" );
         $lob = new Lob( self::$db, self::$cl );
         $oid = $this->getOid();
         $err = $lob->open( $oid, SDB_LOB_CREATEONLY );
         $this->assertEquals( 0, $err ) ;
         
         $err = $lob->seek( 1024, SDB_LOB_END );
         $this->assertEquals( -6, $err ) ;
         $err=$lob->write( 1024 );
         $this->assertEquals( -6, $err ) ;
         $lob->close();
      }
      
      /**
        * @depends testWrite7683
        * 
        */
      public function testSeek7687($oid)
      {
         var_dump("testSeekForRead");
         $lob = new Lob( self::$db, self::$cl );
         $err = $lob->open( $oid, SDB_LOB_READ );
         $this->assertEquals( 0, $err ) ;
         $err = $lob->read( 512 );
         $this->assertEquals( 0, $err ) ;
         
         $err = $lob->seek( 0, SDB_LOB_SET );
         $this->assertEquals( 0, $err ) ;
         
         //$err = $lob->seek(511, SDB_LOB_SET);
         //$this->assertEquals( 0, $err) ;
         $err = $lob->read( 512 );
         $this->assertEquals( 0, $err ) ;
         
         $err = $lob->closeLob();
         $this->assertEquals( 0, $err ) ;
         
         $this->rbuf = $lob->getRContent();
         $ret =  ( self::$wmd5 == md5( $this->rbuf ) );
         $this->assertEquals( true, $ret ) ;
 
      }
        
      public static function tearDownAfterClass()
      {
         self::$cs->drop();
         $err = self::$db->close();
      }
      
   }
?>

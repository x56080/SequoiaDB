<?php
class lob_Test extends PHPUnit_Framework_TestCase
{
   protected $hostname ;
   protected $port ;
   protected $address ;

   protected function setUp()
   {
      $this -> hostname = empty( $_POST['hostname'] ) ? '127.0.0.1' : $_POST['hostname'] ;
      $this -> port = empty( $_POST['port'] ) ? '11810' : $_POST['port'] ;
      $this -> address = $this -> hostname.':'.$this -> port ;
   }

   public function test_Connect()
   {
      $db = new SequoiaDB();
      $err = $db -> connect( $this->address ) ;
      $this -> assertEquals( 0, $err['errno'], '数据库连接错误( 参数: '.$this->address.' )' ) ;
      return $db ;
   }

   /**
    * @depends test_Connect
    */
   public function test_select_cs( $db )
   {
      $cs = $db -> selectCS( 'test_foo' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cs错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cs错误' ) ;
      return $cs ;
   }

   /**
    * @depends test_Connect
    * @depends test_select_cs
    */
   public function test_select_cl( $db, $cs )
   {
      $cl = $cs -> selectCL( 'test_bar', array( 'ReplSize' => -1 ) ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], '创建cl错误' ) ;
      $this -> assertNotEmpty( $cs, '创建cl错误' ) ;
      return $cl ;
   }
     
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_openLob_createModel( $db, $cs, $cl )
   {
      $lob = $cl -> openLob( "123456789012345678901234", SDB_LOB_CREATEONLY ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'openLob错误' ) ;
      $this -> assertNotEmpty( $lob, 'openLob错误' ) ;
      return $lob ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_openLob_createModel
    */
   public function test_write( $db, $lob )
   {
      $err = $lob -> write( "123456" ) ;
      $this -> assertEquals( 0, $err['errno'], 'write错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_openLob_createModel
    */
   public function test_closeLob1( $db, $lob )
   {
      $err = $lob -> close() ;
      $this -> assertEquals( 0, $err['errno'], 'Lob close错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_openLob_readModel( $db, $cs, $cl )
   {
      $lob = $cl -> openLob( "123456789012345678901234", SDB_LOB_READ ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'openLob错误' ) ;
      $this -> assertNotEmpty( $lob, 'openLob错误' ) ;
      return $lob ;
   }
   
        
   /**
    * @depends test_Connect
    * @depends test_openLob_readModel
    */
   public function test_getCreateTime( $db, $lob )
   {
      $timer = $lob -> getCreateTime() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'getCreateTime错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_openLob_readModel
    */
   public function test_getSize( $db, $lob )
   {
      $size = $lob -> getSize() ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'getSize错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_openLob_readModel
    */
   public function test_read( $db, $lob )
   {
      $str = $lob -> read( 3 ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'read错误' ) ;
      $this -> assertEquals( "123", $str, 'read错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_openLob_readModel
    */
   public function test_seek( $db, $lob )
   {
      $err = $lob -> seek( 2, SDB_LOB_SET ) ;
      $this -> assertEquals( 0, $err['errno'], 'seek错误' ) ;
      $str = $lob -> read( 1 ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'read错误' ) ;
      $this -> assertEquals( "3", $str, 'read错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_openLob_readModel
    */
   public function test_closeLob2( $db, $lob )
   {
      $err = $lob -> close() ;
      $this -> assertEquals( 0, $err['errno'], 'Lob close错误' ) ;
   }
   
   /**
    * @depends test_Connect
    * @depends test_select_cs
    * @depends test_select_cl
    */
   public function test_removeLob( $db, $cs, $cl )
   {
      $err = $cl -> removeLob( "123456789012345678901234" ) ;
      $this -> assertEquals( 0, $err['errno'], 'removeLob错误' ) ;
      $err = $db -> getError() ;
      $this -> assertEquals( 0, $err['errno'], 'removeLob错误' ) ;
   }
}
?>

/****************************************************
@description:      evalJs
@testlink cases:   seqDB-7703 
@modify list:
        2016-6-13 wenjing wang init
****************************************************/
<?php
   define('Cur_Path', dirname(__FILE__));
   include_once Cur_Path.'/lib/comm.php';
   include_once Cur_Path.'/../global.php';
class evalJsTest extends PHPUnit_Framework_TestCase
{
    protected $db;
    protected function setUp()
    {
       $this->db = new Sequoiadb() ;
       $err = $this->db->connect( globalParameter::getHostName() , 
                                  globalParameter::getCoordPort() ) ;
       $this->assertEquals( 0, $err['errno'] ) ;
       if ( common::IsStandlone( $this->db ) ) 
       {
          $this->markTestSkipped('database is standlone') ; 
       }
    }
    
    public function testEvalJS()
    {
       $err = $this->db->createJsProcedure( 'function sum7703( a,b ){ return a + b ; }' ) ;
       $this->assertEquals( 0, $err['errno'] ) ;
       
       $result = $this->db->evalJs( 'sum7703( 1, 2 );' ) ;
       $this->assertEquals( 3, $result ) ; 
       $err = $this->db->removeProcedure( 'sum7703' ) ;
       $this->assertEquals( 0, $err['errno'] ) ; 
    }
    
    protected function tearDown()
    {
        $err = $this->db->close();
        $this->assertEquals( 0, $err['errno'] ) ;
    }
}
?>

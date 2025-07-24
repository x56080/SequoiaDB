/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = select_drop_Collection_option_test.php

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
<?php
class select_drop_Collection_option_test extends PHPUnit_Framework_TestCase
{
	public function testselectCS()
	{
		$sdb = new SecureSdb() ;
		$array = $sdb->connect( "localhost:11810" ) ;
		$this->assertEquals( 0, $array['errno'] ) ;
		$cs = $sdb->selectCS( "cs_test" ) ;
		$this->assertNotEmpty( $cs ) ;
		return $cs ;
	}
	/**
	* @depends testselectCS
	*/
	public function testselectCL(SequoiaCS $cs)
	{
		$cl = $cs->selectCollection( "cl_test", '{ ShardingKey:{id:1}, ShardingType:"range", ReplSize:0, Compressed:true }' ) ;
		$this->assertNotEmpty( $cl ) ;
		
		$array_option = array(
			"ShardingKey" => array( "id" => 1),
			"ShardingType" => "range",
			"ReplSize" => 0,
			"Compressed" => true ) ;
		$cl2 = $cs->selectCollection( "cl_test2", $array_option) ;
		$this->assertNotEmpty( $cl2 ) ;
		return $cs ;
	}
	/**
	* @depends testselectCL
	*/
	public function testdropCollection(SequoiaCS $cs)
	{
		$array = $cs->dropCollection( "cl_test" ) ;
		$this->assertEquals( 0, $array["errno"] ) ;
		$array = $cs->dropCollection( "cl_test2" );
		$this->assertEquals( 0, $array["errno"] ) ;
		return $cs ;
		
	}
	/**
	* @depends testdropCollection
	*/
	public function testdrop(SequoiaCS $cs)
	{
		$array = $cs->drop() ;
		$this->assertEquals( 0, $array["errno"] ) ;
	}
}
?>
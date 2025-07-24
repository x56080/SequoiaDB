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

   Source File Name = installTest.php

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
class installTest extends PHPUnit_Framework_TestCase
{
	public function testconnect()
	{
		$sdb = new Sequoiadb() ;
		$array = $sdb->connect("localhost:11810") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		return $sdb ;
	}
	/**
	* @depends testconnect
	*/
	public function testinstall(SequoiaDB $sdb)
	{
		$array_install = array( "install" => false ) ;
		$sdb->install( $array_install ) ;
		$sdb->selectCS( "cs_test" ) ;
		$str = $sdb->getError() ;
		$this->assertEquals( '{"errno":0}', $str ) ;
		
		$array_install = array( "install" => true ) ;
		$sdb->install( $array_install ) ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$sdb->install( '{ "install":false }' ) ;
		$str = $sdb->dropCollectionSpace( "cs_test" ) ;
		$this->assertEquals( '{"errno":0}', $str ) ;
		
		$sdb->install( '{ "install":true }' ) ;
		$array = $sdb->dropCollectionSpace( "cs_test" ) ;
		$this->assertEquals( -34, $array["errno"] ) ;
		$sdb->install( '{ "install":false }' ) ;
		$str = $sdb->getError() ;
		$this->assertEquals( '{"errno":-34}', $str ) ;
		
	}
}
?>

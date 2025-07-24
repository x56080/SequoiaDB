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

   Source File Name = exec_and_execUpdateTest.php

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
class exec_and_execUpdateCSTest extends PHPUnit_Framework_TestCase
{
	public function testconnect()
	{
		$sdb = new Sequoiadb() ;
		$array = $sdb->connect( "localhost:11810" ) ;
		$this->assertEquals( 0, $array['errno'] ) ;
		return $sdb ;
	}
	/**
	* @depends testconnect
	*/
	public function testexecUpdate( SequoiaDB $sdb )
	{
		$array = $sdb->execUpdateSQL("create collectionspace cs_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		$array = $sdb->execUpdateSQL("drop collectionspace cs_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		$array = $sdb->execUpdateSQL("create collectionspace cs_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$array = $sdb->execUpdateSQL("create collection cs_test.cl_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		$array = $sdb->execUpdateSQL("drop collection cs_test.cl_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		$array = $sdb->execUpdateSQL("create collection cs_test.cl_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$array = $sdb->execUpdateSQL("create index test_index on cs_test.cl_test (age)") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		$array = $sdb->execUpdateSQL("drop index test_index on cs_test.cl_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		for( $i = 0; $i < 100; ++$i)
		{
			$array = $sdb->execUpdateSQL("insert into cs_test.cl_test(age,name) values(20,\"Tom\")") ;
			$this->assertEquals( 0, $array['errno'] ) ;
		}
		$array = $sdb->execUpdateSQL("update cs_test.cl_test set age=25") ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		return $sdb;
	}
	/**
	* @depends testexecUpdate
	*/
	public function testexec(SequoiaDB $sdb)
	{
		$sdb->execSQL("list collections") ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$sdb->execSQL("select * from cs_test.cl_test") ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$sdb->execSQL("list collectionspaces") ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$sdb->execSQL("select age,count(name) as Ա from cs_test.cl_test group by age ") ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$sdb->execSQL("select * from cs_test.cl_test order by age desc") ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$sdb->execSQL("select * from cs_test.cl_test limit 5") ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
		
		$sdb->execSQL("select * from cs_test.cl_test offset 5") ;
		$array = $sdb->getError() ;
		$this->assertEquals( 0, $array['errno'] ) ;
	}
	/**
	* @depends testexecUpdate
	*/
	public function testdrop(SequoiaDB $sdb)
	{
		$array = $sdb->execUpdateSQL("drop collectionspace cs_test") ;
		$this->assertEquals( 0, $array['errno'] ) ;
	}
	protected function onNotSuccessfulTest( Exception $e )
	{
		$sdb = new Sequoiadb() ;
		$array = $sdb->connect( "localhost:11810" ) ;
		$this->assertEquals( 0, $array['errno'] ) ;
		$array = $sdb->execUpdateSQL("drop collectionspace cs_test") ;
		fwrite( STDOUT, __METHOD__ . "\n" ) ;
		throw $e ;
	}
}
?>

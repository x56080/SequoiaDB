/* *****************************************************************************
@Description: attach hashCL and insert and remove
@modify list:
   2014-07-30 pusheng Ding  Init
***************************************************************************** */
var csName = COMMCSNAME;
var MainCL_Name = CHANGEDPREFIX + "_year";
var subCl_Name1 = CHANGEDPREFIX + "_month1";
var subCl_Name2 = CHANGEDPREFIX + "_month2";

// Not Error, test mainCL'ShardingType is range and subCL's ShardingType is hash ,insert large number's result
function test_range_attach_hash_insert_remove () 
{
	try
	{
		commDropCL( db, csName, subCl_Name1, true, true,
			"clean sub collection" );
		commDropCL( db, csName, subCl_Name2, true, true,
			"clean sub collection" );
		commDropCL( db, csName, MainCL_Name, true, true,
			"clean main collection" );
	}
	catch( e )
	{
		println( "failed to drop main and sub cl, rc = " + e );
		throw e;
	}

	try 
	{
		db.setSessionAttr( { PreferedInstance: "M" } );
		var cs = commCreateCS( db, csName, true, "create cs in the beginning" );
	}
	catch( e )
	{
		println( "failed to create cs, rc = " + e );
		throw e;
	}

	try
	{
		var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1, b: -1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true } );
		println( "mainCL" );
		var subCL1 = cs.createCL( subCl_Name1, { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
		println( "subCL1" );
		var subCL2 = cs.createCL( subCl_Name2, { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
		println( "subCL2" );
		mainCL.attachCL( csName + "." + subCl_Name1, { LowBound: { a: 0, b: 1000 }, UpBound: { a: 1000, b: 0 } } );
		println( "attach subCL1" );
		mainCL.attachCL( csName + "." + subCl_Name2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
		println( "attach subCL2" );
	}
	catch( e )
	{
		throw e;
	}

	try 
	{
		println( "first remove test" );
		mainCL.remove();
		for( var i = 0; i < 2000; ++i )
		{
			mainCL.insert( { a: i } );
		}
		mainCL.remove();
		//check results
		var mainCnt = mainCL.find().count();
		if( 0 !== parseInt( mainCnt ) )
		{
			println( "Failed to check results. Expect mainCnt: 0, actual mainCnt: " + parseInt( mainCnt ) );
		}

		println( "second remove test" );
		subCL1.remove();
		subCL2.remove();
		mainCL.remove();
		for( var i = 0; i < 2000; ++i )
		{
			mainCL.insert( { a: i } );
		}
		subCL1.remove();
		subCL2.remove();
		//check results
		var mainCnt = mainCL.count();
		if( 0 !== parseInt( mainCnt ) )
		{
			println( "Failed to check results. Expect mainCnt: 0, actual mainCnt: " + parseInt( mainCnt ) );
		}

		println( "third remove test" );
		mainCL.remove();
		for( var i = 0; i < 2000; ++i )
		{
			mainCL.insert( { a: i } );
		}
		subCL1.remove();
		//check results
		var subCnt1 = subCL1.count();
		var mainCnt1 = mainCL.find( { a: { $gte: 1000 } } ).count();
		var mainCnt2 = mainCL.count();
		if( 0 !== parseInt( subCnt1 ) || 1000 !== parseInt( mainCnt1 )
			|| parseInt( mainCnt1 ) !== parseInt( mainCnt ) )
		{
			println( "Failed to check results. Expect [subCnt1,mainCnt1,mainCnt2: 0,1000,1000], actual [subCnt1,mainCnt1,mainCnt2: "
				+ parseInt( subCL1 ) + "," + parseInt( mainCnt1 ) + "," + parseInt( mainCnt2 ) + "]" );
		}

		println( "forth remove test" );
		for( var i = 0; i < 2000; ++i )
		{
			mainCL.insert( { a: i } );
		}
		subCL2.remove();
		//check results
		var subCnt2 = subCL2.count();
		var mainCnt1 = mainCL.find( { a: { $lt: 1000 } } ).count();
		var mainCnt2 = mainCL.count();
		if( 0 !== parseInt( subCnt2 ) || 1000 !== parseInt( mainCnt1 )
			|| parseInt( mainCnt1 ) !== parseInt( mainCnt ) )
		{
			println( "Failed to check results. Expect [subCnt2,mainCnt1,mainCnt2: 0,1000,1000], actual [subCnt2,mainCnt1,mainCnt2: "
				+ parseInt( subCL2 ) + "," + parseInt( mainCnt1 ) + "," + parseInt( mainCnt2 ) + "]" );
		}

		println( "firth remove test" );
		mainCL.remove();
		subCL1.remove();
		subCL2.remove();
		//check results
		var mainCnt = mainCL.find().count();
		if( 0 !== parseInt( mainCnt ) )
		{
			println( "Failed to check results. Expect mainCnt: 0, actual mainCnt: " + parseInt( mainCnt ) );
		}
	}
	catch( e )
	{
		println( "Failed to remove CL records." );
		throw e;
	}

}

try
{
	// Inspect the run mode is standalone or not
	if( true == commIsStandalone( db ) )
		throw "ModeStandAlone";
	test_range_attach_hash_insert_remove();

	commDropCL( db, csName, subCl_Name1, false, false,
		"Failed to clean subCL1 in the end." );
	commDropCL( db, csName, subCl_Name2, false, false,
		"Failed to clean subCL2 in the end." );
	commDropCL( db, csName, MainCL_Name, false, false,
		"Failed to clean mainCL in the end." );
}
catch( e )
{
	if( "ModeStandAlone" == e )
		println( "The run mode is standalone" );
	else
		throw e;
}
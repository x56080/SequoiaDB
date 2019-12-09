/* *****************************************************************************
@Description: attach hashCL and insert-BoundData
@modify list:
   2014-07-30 pusheng Ding  Init
***************************************************************************** */

function test_range_attach_hash_insert_BoundData () // Not Error, test mainCL'ShardingType is range and subCL's ShardingType is hash ,insert BoundData's result
{
	MainCL_Name = CHANGEDPREFIX + "year";
	subCl_Name = CHANGEDPREFIX + "month";
	try
	{
		commDropCL( db, COMMCSNAME, subCl_Name + "1", true, true,
			"clean sub collection" );
		commDropCL( db, COMMCSNAME, subCl_Name + "2", true, true,
			"clean sub collection" );
		commDropCL( db, COMMCSNAME, MainCL_Name, true, true,
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
		var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );
	} catch( e )
	{
		println( "failed to create cs, rc = " + e );
		throw e;
	}
	println( COMMCSNAME );

	try
	{
		var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1, b: -1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true } );
		println( "mainCL" );
		var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
		println( "subCL1" );
		var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
		println( "subCL2" );
		mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "1", { LowBound: { a: 0 }, UpBound: { a: 1 } } );
		println( "attach subCL1" );
		mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "2", { LowBound: { a: 1 }, UpBound: { a: 2 } } );
		println( "attach subCL2" );
	}
	catch( e )
	{
		throw e;
	}

	try
	{
		var subCL = [];
		subCL.push( subCL1 );
		subCL.push( subCL2 );
		var numberOfsubCl = 2;
		for( var i = 0; i < numberOfsubCl; ++i )
		{
			var sourceDataGroupName = getSourceGroupName_alone( COMMCSNAME, subCl_Name + ( i + 1 ) );
			println( "sourceDataGroupName is : " + sourceDataGroupName );

			var desDataGroupName = getOtherDataGroups( sourceDataGroupName );
			println( "desDataGroupName is " + desDataGroupName );

			var Partition = getPartition( COMMCSNAME, subCl_Name + ( i + 1 ) );
			println( "Partition is : " + Partition );

			if( !subCL_split_hash( subCL[i], sourceDataGroupName, desDataGroupName, Partition ) )
			{
				println( "************SPLIT SUCCED***************" );
			}
		}
	}
	catch( e )
	{
		println( " Error: " + e );
		throw e;
	}

	try // smaller than LowBound
	{
		mainCL.insert( { a: -1 } );
		throw "Error. insert data out of range successfull.";
	}
	catch( e )
	{
		if( e != -135 )
		{
			println( "smaller than LowBound i = -1 " + ", err is :" + e );
			throw e;
		}
	}

	try // bigger than UpBound
	{
		mainCL.insert( { a: 2 } );
		throw "Error. insert data out of range successfull.";
	}
	catch( e )
	{
		if( e != -135 )
		{
			println( "bigger than UpBound i = 2 " + ", err is :" + e );
			throw e;
		}
	}

	try //insert lots of same data into subCL1
	{
		for( var i = 0; i < 1000; ++i )
		{
			mainCL.insert( { a: 0 } );
			mainCL.insert( { a: 1, b: i } );
		}
	}
	catch( e )
	{
		println( "Error insert lots of same data into subCL1 ,i = " + i + ", err is :" + e );
		throw e;
	}

	try //insert lots of same data into subCL2
	{
		for( var i = 0; i < 1000; ++i )
		{
			mainCL.insert( { a: 1 } );
			mainCL.insert( { a: 2, b: i } );
		}
	}
	catch( e )
	{
		println( "Error insert lots of same data into subCL2 ,i = " + i + ", err is :" + e );
		throw e;
	}

	//insert records
	try
	{
		var mainCnt = mainCL.count();
		var subCLCnt1 = subCL1.count();
		var subCLCnt2 = subCL2.count();
	}
	catch( e )
	{
		println( "Failed to insert records. rc=" + e );
		throw e;
	}
	//check results
	var sumCnt = 0;
	var sumCnt = subCLCnt1 + subCLCnt2;
	if( parseInt( mainCnt ) !== parseInt( sumCnt ) || parseInt( mainCnt ) !== 4000 )
	{
		println( "Failed to check results." );
		throw -1;
	}
}

// Add inspect standalone run mode
try
{
	// Inspect the run mode is standalone or not
	if( true == commIsStandalone( db ) )
		throw "ModeStandAlone";

	test_range_attach_hash_insert_BoundData();

	//clean
	println( "---Begin to clean CL in the end." );
	commDropCL( db, COMMCSNAME, MainCL_Name, false, false,
		"drop mainCL in the end." );
}
catch( e )
{
	if( "ModeStandAlone" == e )
		println( "The run mode is standalone" );
	else
		throw e;
}
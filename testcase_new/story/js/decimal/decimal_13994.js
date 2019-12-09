/******************************************************************************
*@Description : test find special decimal value with func symbol
*               $abs $ceiling $floor $mod $add $subtract 
*               $multiply $divide $cast
*               seqDB-13994:使用函数操作符查询特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
	var docs = [{ a: { $decimal: "MAX" } },
	{ a: { $decimal: "MIN" } },
	{ a: { $decimal: "NaN" } }];

	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
	var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );
	insertData( cl, docs );

	// test $abs -6 ?
	println( "test $abs" );
	testFindData( cl, {}, { a: { $abs: 1 } }, -6 );

	println( "test $ceiling" );
	testFindData( cl, {}, { a: { $ceiling: { $decimal: "MAX" } } }, -6 );

	cursor = findData( cl, {}, { a: { $ceiling: 1 } } );
	expRecs = [{ a: { $decimal: "NaN" } },
	{ a: { $decimal: "NaN" } },
	{ a: { $decimal: "NaN" } }];
	checkRec( cursor, expRecs );

	println( "test $floor" );
	cursor = findData( cl, {}, { a: { $floor: 1 } } );
	checkRec( cursor, expRecs );

	println( "test $mod" );
	cursor = findData( cl, {}, { a: { $mod: 2 } } );
	checkRec( cursor, expRecs );

	println( "test $add" );
	cursor = findData( cl, {}, { a: { $add: 1 } } );
	checkRec( cursor, expRecs );

	println( "test $subtract" );
	cursor = findData( cl, {}, { a: { $subtract: 1 } } );
	checkRec( cursor, expRecs );

	println( "test $multiply" );
	cursor = findData( cl, {}, { a: { $multiply: 1 } } );
	checkRec( cursor, expRecs );

	println( "test $divide" );
	cursor = findData( cl, {}, { a: { $divide: 1 } } );
	checkRec( cursor, expRecs );

	println( "test $cast" );
	testCast( cl );
}

function testFindData ( cl, cond, sel, errno )
{
	try
	{
		var cursor = cl.find( cond, sel );
		cursor.next();
		throw 0;
	}
	catch( e )
	{
		if( e !== errno )
		{
			throw buildException( "testFindData", e, "find data", errno, e );
		}
	}
}

function testCast ( cl )
{
	println( "cast MinKey" );
	var cursor = findData( cl, {}, { a: { $cast: "minkey" } } );
	var expRecs = [{ a: { $minKey: 1 } },
	{ a: { $minKey: 1 } },
	{ a: { $minKey: 1 } }];
	checkRec( cursor, expRecs );

	//cast Double
	/*
	cursor = sortFindData( cl, {}, { a: { $cast: "double" } }, { _id: 1 } ) ;
	expRecs = [ { a: 0 },
	            { a: 0 },
	            { a: "nan" } ] ;
	checkRec( cursor, expRecs );
	*/

	println( "cast String" );
	cursor = sortFindData( cl, {}, { a: { $cast: "string" } }, { _id: 1 } );
	expRecs = [{ a: "MAX" },
	{ a: "MIN" },
	{ a: "NaN" }];
	checkRec( cursor, expRecs );

	println( "cast Bool" );
	cursor = sortFindData( cl, {}, { a: { $cast: "bool" } }, { _id: 1 } );
	expRecs = [{ a: true },
	{ a: true },
	{ a: true }];
	checkRec( cursor, expRecs );

	println( "cast Date" );
	cursor = sortFindData( cl, {}, { a: { $cast: "date" } }, { _id: 1 } );
	expRecs = [{ a: null },
	{ a: null },
	{ a: null }];
	checkRec( cursor, expRecs );

	println( "cast Null" );
	cursor = sortFindData( cl, {}, { a: { $cast: "null" } }, { _id: 1 } );
	expRecs = [{ a: null },
	{ a: null },
	{ a: null }];
	checkRec( cursor, expRecs );

	println( "cast Int32" );
	cursor = sortFindData( cl, {}, { a: { $cast: "int32" } }, { _id: 1 } );
	expRecs = [{ a: 0 },
	{ a: 0 },
	{ a: 0 }];
	checkRec( cursor, expRecs );

	println( "cast Timestamp" );
	cursor = sortFindData( cl, {}, { a: { $cast: "timestamp" } }, { _id: 1 } );
	expRecs = [{ a: null },
	{ a: null },
	{ a: null }];
	checkRec( cursor, expRecs );

	println( "cast Int64" );
	cursor = sortFindData( cl, {}, { a: { $cast: "int64" } }, { _id: 1 } );
	expRecs = [{ a: 0 },
	{ a: 0 },
	{ a: 0 }];
	checkRec( cursor, expRecs );

	println( "cast MaxKey" );
	cursor = sortFindData( cl, {}, { a: { $cast: "maxkey" } }, { _id: 1 } );
	expRecs = [{ a: { $maxKey: 1 } },
	{ a: { $maxKey: 1 } },
	{ a: { $maxKey: 1 } }];
	checkRec( cursor, expRecs );
}
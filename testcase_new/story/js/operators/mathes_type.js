/* *****************************************************************************
@discretion: operators basic: $type
@modify list:
   						2014-01-29 Pusheng Ding  Init
***************************************************************************** */
csName = COMMCSNAME;
clName = CHANGEDPREFIX + "_mathes";

println( "------Begin to ready env." );
try
{
	commDropCL( db, csName, clName, true, true, "drop cl in the begin" );
	var opt = { ReplSize: 0, Compressed: true };
	var varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
}
catch( e )
{
	println( "Failed to drop/create cl in the begin." );
	throw e;
}

println( "------Begin to insert into records." );
try
{
	//32-bit integer
	varCL.insert( { a: 214748364, b: 16 } );
	//64-bit integer
	varCL.insert( { a: NumberLong( 42474836478 ), b: 18 } );
	//double
	varCL.insert( { a: 1.5e+100, b: 1 } );
	//string
	varCL.insert( { a: 'abcd', b: 2 } );
	//ObjectID
	varCL.insert( { a: { $oid: '5156c192f970aed30c020000' }, b: 7 } );
	//boolean
	varCL.insert( { a: true, b: 8 } );
	//date
	varCL.insert( { a: { $date: '2014-08-11' }, b: 9 } );
	//timestamp
	varCL.insert( { a: { $timestamp: '2014-08-11-15.35.44.123456' }, b: 17 } );
	//Binary data
	varCL.insert( { a: { $binary: 'aGVsbG8gd29ybGQ=', $type: '1' }, b: 5 } );
	//Regular expression
	varCL.insert( { a: { $regex: '^W', $options: 'i' }, b: 11 } );
	//Object
	varCL.insert( { a: { a1: 'object' }, b: 3 } );
	//Array
	varCL.insert( { a: [1, 2, 3, 4], b: 4 } );
	//null
	varCL.insert( { a: null, b: 10 } );
	varCL.insert( { b: 10 } );
} catch( e )
{
	println( "Failed to insert records." );
	throw e;
}

//type:16  -----jira: SEQUOIADBMAINSTREAM-1580
println( "------Begin to check results." );
try
{
	var typeAll = new Array;
	var typeAll = [18, 1, 2, 7, 8, 9, 17, 5, 11, 3, 4, 10
		//	,16
	];
	for( i = 0; i < typeAll.length; i++ )
	{
		var type = typeAll[i];
		var findCnt = varCL.find( { a: { $type: 1, $et: type } } ).count();
		var findRst = varCL.find( { a: { $type: 1, $et: type } } ).current().toObj()["b"];
		if( findCnt != 1 || findRst != type )
		{
			throw ( "Failed to check results. Failed type is " + type );
		}
	}
}
catch( e )
{
	println( "Failed to find use the type." );
	throw e;
}

try
{
	commDropCL( db, csName, clName, true, false, "Failed to drop CL." );
}
catch( e )
{
	throw e;
}
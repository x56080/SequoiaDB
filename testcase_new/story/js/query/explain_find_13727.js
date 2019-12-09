/******************************************************************************
@Description : 1. explain:{Run:true} or explain:{Run:false}
@Modify list :
               2014-11-11 pusheng Ding  Init
******************************************************************************/

if( false == commIsStandalone( db ) )
{
	// set session get data from master
	db.setSessionAttr( { "PreferedInstance": "M" } );
}

try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
	println( "unexpected err happened when clear cs:" + e );
	throw e;
}

//create CS
try
{
	var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
}
catch( e )
{
	println( "failed to create cs,rc=" + e );
	throw e;
}

//create CL
try
{
	var varCL = varCS.createCL( COMMCLNAME );
} catch( e )
{
	println( "can't create normal-CL:" + COMMCLNAME + " rc=" + e );
	throw e;
}
println( "create " + COMMCLNAME + " finished" );

//insert data
try
{
	for( var i = 0; i < 5; i++ ) { varCL.insert( { a: i - 5, b: i, c: "abcdefghijkl" + i } ); }
} catch( e )
{
	println( "insert-data to " + COMMCLNAME + " fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//find().explain()
//default is explain({Run:false})
println( COMMCLNAME + ".find().explain() begin..." );
try
{
	var expl = varCL.find().explain().toArray();
	var obj = eval( '(' + expl + ')' );
	if( obj['ReturnNum'] != 0 )
	{
		println( "explain's ReturnNum is not 0 while Run is default value!" );
		throw -1;
	}
	if( obj['ScanType'] != 'tbscan' )
	{
		println( "explain's ScanType is not tbscan while there is not index matched!" );
		throw -1;
	}
	if( obj['UseExtSort'] != false )
	{
		println( "explain's UseExtSort is not false while there is not a sort sub!" );
		throw -1;
	}
	if( obj['Name'] != ( COMMCSNAME + '.' + COMMCLNAME ) )
	{
		println( "explain's Name is error!" );
		throw -1;
	}
	if( obj['IndexName'] != '' )
	{
		println( "explain's IndexName is not empty while query wouldn't use any index!" );
		throw -1;
	}
	if( obj['IndexRead'] != 0 || obj['DataRead'] != 0 )
	{
		println( "explain's statistics are error!" );
		throw -1;
	}
} catch( e )
{
	if( e != -1 )
	{
		println( COMMCLNAME + ".find().explain() fail! rc=" + e + ", expl is: " + expl );
	}
	else
	{
		println( "explain is not expected.Returned explain:" );
		println( expl );
	}

	throw e;
}
println( COMMCLNAME + ".find().explain() finished" );

//find().explain({Run:true})
println( COMMCLNAME + ".find().explain({Run:true}) begin..." );
try
{
	var clName = COMMCSNAME + "." + COMMCLNAME;
	var primaryNode = queryGetPrimaryNode( db, clName );
	if( false == commIsStandalone( db ) )
	{
		println( "====>" );
		println( "before explain, primary node: " + primaryNode[0] + ":" +
			primaryNode[1] );
		var dataDB = new Sdb( primaryNode[0], primaryNode[1] );
		var dataCS = dataDB.getCS( COMMCSNAME );
		var dataCL = dataCS.getCL( COMMCLNAME );
		println( "before explain, query from master: " + dataCL.count() );
		println( "<====" );
	}
	var expl = varCL.find().explain( { Run: true } ).toArray();
	var obj = eval( '(' + expl + ')' );
	if( obj['ReturnNum'] != 5 )
	{
		println( "explain's ReturnNum is not 5 while Run is default value!" );
		throw -1;
	}
	if( obj['ScanType'] != 'tbscan' )
	{
		println( "explain's ScanType is not tbscan while there is not index matched!" );
		throw -1;
	}
	if( obj['UseExtSort'] != false )
	{
		println( "explain's UseExtSort is not false while there is not a sort sub!" );
		throw -1;
	}
	if( obj['Name'] != ( COMMCSNAME + '.' + COMMCLNAME ) )
	{
		println( "explain's Name is error!" );
		throw -1;
	}
	if( obj['IndexName'] != '' )
	{
		println( "explain's IndexName is not empty while query wouldn't use any index!" );
		throw -1;
	}
	if( obj['IndexRead'] != 0 || obj['DataRead'] == 0 )
	{
		println( "explain's statistics are error!" );
		throw -1;
	}
} catch( e )
{
	if( e != -1 )
	{
		println( "====>" );
		var primaryNode = queryGetPrimaryNode( db, clName );
		println( "after explain, primary node: " + primaryNode[0] + ":" +
			primaryNode[1] );
		dataDB = new Sdb( primaryNode[0], primaryNode[1] );
		dataCS = dataDB.getCS( COMMCSNAME );
		dataCL = dataCS.getCL( COMMCLNAME );
		println( "after explain, query from master: " + dataCL.count() );
		println( "<====" );
		println( COMMCLNAME + ".find().explain({Run:true}) fail! rc=" + e );
		println( "query number: " + varCL.find().count() );
	}
	else
	{
		println( "explain is not expected.Returned explain:" );
		println( expl );
	}

	throw e;
}
println( COMMCLNAME + ".find().explain({Run:true}) finished" );

//find().explain({Run:false})
//default is explain({Run:false})
println( COMMCLNAME + ".find().explain({Run:false}) begin..." );
try
{
	var expl = varCL.find().explain( { Run: false } ).toArray();
	var obj = eval( '(' + expl + ')' );
	if( obj['ReturnNum'] != 0 )
	{
		println( "explain's ReturnNum is not 0 while Run is default value!" );
		throw -1;
	}
	if( obj['ScanType'] != 'tbscan' )
	{
		println( "explain's ScanType is not tbscan while there is not index matched!" );
		throw -1;
	}
	if( obj['UseExtSort'] != false )
	{
		println( "explain's UseExtSort is not false while there is not a sort sub!" );
		throw -1;
	}
	if( obj['Name'] != ( COMMCSNAME + '.' + COMMCLNAME ) )
	{
		println( "explain's Name is error!" );
		throw -1;
	}
	if( obj['IndexName'] != '' )
	{
		println( "explain's IndexName is not empty while query wouldn't use any index!" );
		throw -1;
	}
	if( obj['IndexRead'] != 0 || obj['DataRead'] != 0 )
	{
		println( "explain's statistics are error!" );
		throw -1;
	}
} catch( e )
{
	if( e != -1 )
	{
		println( COMMCLNAME + ".find().explain({Run:false}) fail! rc=" + e );
	}
	else
	{
		println( "explain is not expected.Returned explain:" );
		println( expl );
	}

	throw e;
}
println( COMMCLNAME + ".find().explain({Run:false}) finished" );

//clean test-env
try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
		"drop cl in the end" );
} catch( e )
{
	println( "clean test-evn fail! rc=" + e );
	throw e;
}
println( "clean test-evn succ!" );

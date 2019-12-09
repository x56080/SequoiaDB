/******************************************************************************
@Description : 1.query use Operator $in with index
@Modify list :
               2014-08-08 pusheng Ding  Init
******************************************************************************/
CHANGEDPREFIX_IDX = CHANGEDPREFIX + "_idx";

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
	var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );
	println( "create CL finished" );
} catch( e )
{
	//collection already exist,use it
	if( e != -22 )
	{
		println( "can't create CL:" + COMMCLNAME + " rc=" + e );
		throw e;
	}
	else
	{
		varCL = varCS.getCL( COMMCLNAME );
		varCL.remove();
		println( "use CL:" + COMMCLNAME );
	}
}

//insert data
try
{
	for( i = 0; i < 6; i++ )
	{
		varCL.insert( { a: 1 * i, b: 10 * i } );
		varCL.insert( { a: 1 * i, b: 10 * i + 1 } );
	}
} catch( e )
{
	println( "insert data fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//no index
//select * from ... where a in(2) and b!=20
try
{
	var sel = varCL.find( { a: { $in: [2] }, b: { $ne: 20 } } );
	var size = 0;
	var exprows = 1;
	var flag = true;
	while( sel.next() )
	{
		size++;
		if( size > exprows )
		{
			flag = false;
			throw 1;
		}
		var ret = sel.current();
		//expected result:{a:2,b:21}
		if( ret.toObj()['a'] != 2 || ret.toObj()['b'] != 21 )
		{
			flag = false;
			throw 2;
		}
	}
	sel.close();
	if( flag && size != exprows )
	{
		flag = false;
		throw 1;
	}
} catch( e )
{
	if( e != 1 && e != 2 )
	{
		println( "select data fail! rc=" + e );
		throw e;
	}
	else if( e == 1 )
	{
		println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
		throw e;
	}
	else if( e == 2 )
	{
		println( "return incorrect record! expected:{a:2, b:21} returned:" + ret );
		throw e;
	}
}
println( "select data without index finished!" );

//create index
try
{
	varCL.createIndex( CHANGEDPREFIX_IDX, { a: 1 } );
	println( "create index finished" );
} catch( e )
{
	//when redefine index, ignore the exception
	if( e != -247 )
	{
		println( "can't create index:" + CHANGEDPREFIX_IDX + " rc=" + e );
		throw e;
	}
	println( "already exist index:" + CHANGEDPREFIX_IDX );
}
//select * from ... where a in[5,1] and b%10=0
try
{
	var sel = varCL.find( { a: { $in: [5, 1] }, b: { $mod: [10, 0] } } ).sort( { a: 1 } ).hint( { "": CHANGEDPREFIX_IDX } );
	var size = 0;
	var exprows = 2;
	var flag = true;
	while( sel.next() )
	{
		size++;
		if( size > exprows )
		{
			flag = false;
			throw 1;
		}
		var ret = sel.current();
		//expected result:{a:1,b:10} {a:5,b:50}
		if( ret.toObj()['a'] != ( ( size - 1 ) * 4 + 1 ) || ret.toObj()['b'] != ( ( size - 1 ) * 40 + 10 ) )
		{
			flag = false;
			throw 2;
		}
	}
	sel.close();
	if( flag && size != exprows )
	{
		flag = false;
		throw 1;
	}
} catch( e )
{
	if( e != 1 && e != 2 )
	{
		println( "select data fail! rc=" + e );
		throw e;
	}
	else if( e == 1 )
	{
		println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
		throw e;
	}
	else if( e == 2 )
	{
		println( "return incorrect record!" );
		println( "the incorrect record:" + ret );
		throw e;
	}
}
println( "select data with index finished!" );

try
{
	varCL.dropIndex( CHANGEDPREFIX_IDX );
} catch( e )
{
}

//combined index
try
{
	varCL.createIndex( CHANGEDPREFIX_IDX, { a: 1, b: 1 } );
	println( "create index finished" );
} catch( e )
{
	//when redefine index, ignore the exception
	if( e != -247 )
	{
		println( "can't create index:" + CHANGEDPREFIX_IDX + " rc=" + e );
		throw e;
	}
	println( "already exist index:" + CHANGEDPREFIX_IDX );
}

//select * from ... where a>3 and b in(51,60);
try
{
	var sel = varCL.find( { a: { $gt: 3 }, b: { $in: [51, 60] } } ).hint( { "": CHANGEDPREFIX_IDX } );
	var size = 0;
	var exprows = 1;
	var flag = true;
	while( sel.next() )
	{
		size++;
		if( size > exprows )
		{
			flag = false;
			throw 1;
		}
		var ret = sel.current();
		//expected result:{a:5,b:51}
		if( ret.toObj()['a'] != 5 || ret.toObj()['b'] != 51 )
		{
			flag = false;
			throw 2;
		}
	}
	sel.close();
	if( flag && size != exprows )
	{
		flag = false;
		throw 1;
	}
} catch( e )
{
	if( e != 1 && e != 2 )
	{
		println( "select data fail! rc=" + e );
		throw e;
	}
	else if( e == 1 )
	{
		println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
		throw e;
	}
	else if( e == 2 )
	{
		println( "return incorrect record!  expected:{a:5, b:51} returned:" + ret );
		throw e;
	}
}
println( "select data with combined index finished!" );

//clean test-env
try
{
	varCL.dropIndex( CHANGEDPREFIX_IDX );
	commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
} catch( e )
{
	println( "clean test-evn fail! rc=" + e );
	throw e;
}
println( "clean test-evn succ!" );

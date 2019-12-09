/******************************************************************************
@Description : 1.query use Operator $+num with index
@Modify list :
               2014-08-11 pusheng Ding  Init
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
	varCL.insert( { a: [0, 1, 2, 3, 4], b: 0 } );
	varCL.insert( { a: 1, b: 1 } );
	varCL.insert( { a: [2, 3, 4], b: 2 } );
	varCL.insert( { a: [4, 5, 3], b: 3 } );
} catch( e )
{
	println( "insert data fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//no index
//select * from ... where a contains(2}) and b!=0
try
{
	var sel = varCL.find( { "a.$1": 2, b: { $ne: 0 } } ).sort( { b: 1 } );
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
		//expected result:{a:[2,3,4],b:2}
		if( ret.toObj()['b'] != 2 || ret.toObj()['a'] != '2,3,4' )
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
		println( "return incorrect record! expected:{a:[2,3,4],b:2} returned:" + ret );
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
//select * from ... where a contains(1);
try
{
	var sel = varCL.find( { "a.$1": 1 } ).sort( { b: 1 } ).hint( { "": CHANGEDPREFIX_IDX } );
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
		//expected result:{a:[0,1,2,3,4],b:0}
		if( ret.toObj()['a'] != '0,1,2,3,4' || ret.toObj()['b'] != 0 )
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
		println( "return incorrect record! expected:{a:[0,1,2,3,4],b:0} returned:" + ret );
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

//select * from ... where a contains(5) and b>1;
try
{
	var sel = varCL.find( { "a.$3": 5, b: { $gt: 0 } } ).hint( { "": CHANGEDPREFIX_IDX } );
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
		//expected result:{a:[4,5,3],b:3}
		if( ret.toObj()['a'] != '4,5,3' || ret.toObj()['b'] != 3 )
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
		println( "return incorrect record!  expected:{a:[4,5,3],b:3} returned:" + ret );
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

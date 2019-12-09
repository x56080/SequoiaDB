/******************************************************************************
@Description : 1.query use Operator $elemMatch with index
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
	varCL.insert( { a: { a1: 0, a2: 1, a3: 2 }, b: 0 } );
	varCL.insert( { a: { a1: 1, a2: 1, a3: 0 }, b: 1 } );
	varCL.insert( { a: { a1: 2, a2: 1 }, b: 2 } );
	varCL.insert( { a: { a1: 3, a2: 3, a4: 4 }, b: { b1: 0, b2: 3 } } );
	varCL.insert( { a: 4, b: 4 } );
} catch( e )
{
	println( "insert data fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//no index
//select * from ... where a contains({a2:1}) and b!=0
try
{
	var sel = varCL.find( { a: { $elemMatch: { a2: 1 } }, b: { $ne: 0 } } ).sort( { b: 1 } );
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
		//expected result:{a:{a1:1,a2:1,a3:0},b:1} {a:{a1:2,a2:1},b:2}
		if( ret.toObj()['b'] != size )
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
//select * from ... where a contains(a1:3,a2:3) and b contains(b1:0,b2:3});
try
{
	var sel = varCL.find( { a: { $elemMatch: { a1: 3, a2: 3 } }, b: { $elemMatch: { b1: 0, b2: 3 } } } ).hint( { "": CHANGEDPREFIX_IDX } );
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
		//expected result:{a:{a1:3,a2:3,a4:4},b:{b1:0,b2:3}}
		if( ret.toObj()['a']['a1'] != 3 || ret.toObj()['a']['a2'] != 3 || ret.toObj()['a']['a4'] != 4
			|| ret.toObj()['b']['b1'] != 0 || ret.toObj()['b']['b2'] != 3 )
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
		println( "return incorrect record! expected:{a:{a1:3,a2:3,a4:4},b:{b1:0,b2:3}} returned:" + ret );
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
	varCL.createIndex( CHANGEDPREFIX_IDX, { a: 1, b: -1 } );
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

//select * from ... where a contains(a1:2,a2:1) and b>1;
try
{
	var sel = varCL.find( { a: { $elemMatch: { a1: 2, a2: 1 } }, b: { $gt: 0 } } ).hint( { "": CHANGEDPREFIX_IDX } );
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
		//expected result:{a:{a1:2,a2:1},b:2}
		if( ret.toObj()['a']['a1'] != 2 || ret.toObj()['a']['a2'] != 1 || ret.toObj()['b'] != 2 )
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
		println( "return incorrect record!  expected:{a:{a1:2,a2:1},b:2} returned:" + ret );
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

/******************************************************************************
@Description : 1.query use Operator $size with index
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
	varCL.insert( { a: [2, 3], b: 2 } );
	varCL.insert( { a: [4, 5], b: 3 } );
	varCL.insert( { a: [110, 111, 112, 113], b: [0, 1] } );
} catch( e )
{
	println( "insert data fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//no index
//select * from ... where size(a)=0
try
{
	var sel = varCL.find( { a: { $size: 1, $et: 0 } } );
	var size = 0;
	var exprows = 0;
	var flag = true;
	while( sel.next() )
	{
		size++;
		if( size > exprows )
		{
			var ret = sel.current();
			flag = false;
			throw 1;
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
	if( e != 1 )
	{
		println( "select data fail! rc=" + e );
		throw e;
	}
	else if( e == 1 )
	{
		println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
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
//select * from ... where size(a)=4 and size(b)=2;
try
{
	var sel = varCL.find( { a: { $size: 1, $et: 4 }, b: { $size: 1, $et: 2 } } ).hint( { "": CHANGEDPREFIX_IDX } );
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
		//expected result:{a:[110,111,112,113],b:[0,1]}
		if( ret.toObj()['a'] != '110,111,112,113' || ret.toObj()['b'] != '0,1' )
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
		println( "return incorrect record! expected:{a:[110,111,112,113],b:[0,1]} returned:" + ret );
		throw e;
	}
}
println( "select data with index finished!" );

//select * from ... where size(a)=2 and b>0;
try
{
	var sel = varCL.find( { a: { $size: 1, $et: 2 }, b: { $gt: 0 } } ).sort( { b: 1 } ).hint( { "": CHANGEDPREFIX_IDX } );
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
		//expected result:{a:[2,3],b:2} {a:[4,5],b:3}
		if( ret.toObj()['b'] != ( size + 1 ) )
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

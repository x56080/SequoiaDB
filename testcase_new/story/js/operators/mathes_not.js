/* *****************************************************************************
@discretion: operators basic: $not
@modify list:
   						2014-01-29 Pusheng Ding  Init
***************************************************************************** */

try
{
	var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
} catch( e )
{
	println( "can't connect to db" );
	throw e;
}

try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
		"drop cl in the beginning" );
} catch( e ) { }

//create CL
try
{
	var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
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
	for( i = 0; i < 10; i++ )
	{
		varCL.insert( { a: 1 * i, b: 10 * i } );
	}
} catch( e )
{
	println( "insert data fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//no index
//select * from ... where not(a<1 and a>1)
try
{
	var sel = varCL.find( { $not: [{ a: { $lt: 1 } }, { a: { $gt: 1 } }] } ).sort( { a: 1 } );
	var size = 0;
	var exprows = 10;
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
		//expected result:{a:0,b:0} ... {a:9,b:90}
		if( ret.toObj()['a'] != ( size - 1 ) || ret.toObj()['b'] != ( ( size - 1 ) * 10 ) )
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

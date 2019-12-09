/* *****************************************************************************
@discretion: operators basic: $size
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

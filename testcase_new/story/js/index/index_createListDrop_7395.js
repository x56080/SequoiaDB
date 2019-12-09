/******************************************************************************
@Description : 1.createIndex basic
@Modify list :
               2015-01-23  pusheng Ding  Init
******************************************************************************/
CHANGEDPREFIX_IND = CHANGEDPREFIX + "ind";
CHANGEDPREFIX_IND1 = CHANGEDPREFIX + "ind1";

try
{
	var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
} catch( e )
{
	println( "can't connect to db" );
	throw e;
}

db.setSessionAttr( { PreferedInstance: "M" } );
try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
		"drop cl in the beginning" );
} catch( e ) { }

//create CS
try
{
	var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
} catch( e )
{
	println( "can't create CS:" + COMMCSNAME + " rc=" + e );
	throw e;
}
println( "createCS " + COMMCSNAME + " finished" );

//create cl
try
{
	var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 1 } );
} catch( e )
{
	println( "can't create CL:" + COMMCLNAME + " rc=" + e );
	throw e;
}
println( "createCL " + COMMCLNAME + " finished" );

//create index
try
{
	varCL.createIndex( CHANGEDPREFIX_IND, { a: 1 } );
} catch( e )
{
	println( "create index failed! rc=" + e );
	throw e;
}
println( "create index finished" );

//insert data
try
{
	for( var i = 0; i < 100; i++ )
	{
		varCL.insert( { a: 100 - i, b: i, c: "abcdefghijkl" + i } );
	}
} catch( e )
{
	println( "insert-data to " + COMMCLNAME1 + " fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//create index1
try
{
	varCL.createIndex( CHANGEDPREFIX_IND1, { b: 1, a: 1 }, true, false );
} catch( e )
{
	println( "create index1 failed! rc=" + e );
	throw e;
}
println( "create index1 finished" );

//list indexes
try
{
	var sel = varCL.listIndexes();
	var size = 0;
	var exprows = 3;
	var flag = true;
	while( sel.next() )
	{
		size++;
		if( size > exprows )
		{
			flag = false;
			throw "list-indexes-incorrect";
		}
		var ret = sel.current();
	}
	sel.close();
	if( flag && size != exprows )
	{
		flag = false;
		throw "list-indexes-incorrect";
	}
} catch( e )
{
	if( e == "list-indexes-incorrect" )
	{
		println( "list indexes result incorrect!expected 3,but return records number:" + size + ( ( size < exprows ) ? "" : " or more" ) );
		throw e;
	} else
	{
		println( "list indexes failed! rc=" + e );
		throw e;
	}
}
println( "1st list indexes finished!" );

//drop index
try
{
	varCL.dropIndex( CHANGEDPREFIX_IND );
	varCL.dropIndex( CHANGEDPREFIX_IND1 );
} catch( e )
{
	println( "drop indexes failed! rc=" + e );
	throw e;
}
println( "drop indexes finished!" );

//list indexes
try
{
	var sel = varCL.listIndexes();
	var size = 0;
	var exprows = 1;
	var flag = true;
	while( sel.next() )
	{
		size++;
		if( size > exprows )
		{
			flag = false;
			throw "list-indexes-incorrect";
		}
		var ret = sel.current();
	}
	sel.close();
	if( flag && size != exprows )
	{
		flag = false;
		throw "list-indexes-incorrect";
	}
} catch( e )
{
	if( e == "list-indexes-incorrect" )
	{
		println( "list indexes result incorrect!expected 3,but return records number:" + size + ( ( size < exprows ) ? "" : " or more" ) );
		throw e;
	} else
	{
		println( "list indexes failed! rc=" + e );
		throw e;
	}
}
println( "2nd list indexes finished!" );

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

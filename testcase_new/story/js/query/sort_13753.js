/******************************************************************************
@Description : 1. multi-columns sort
@Modify list :
               2015-01-17 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1";
rownum = 6;

try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
} catch( e )
{
	println( "failed to drop cl, rc = " + e );
	throw e;
}

//create CS
try
{
	var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
	var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0 } );
} catch( e )
{
	println( "can't create CS:" + COMMCSNAME + " rc=" + e );
	throw e;
}
println( "createCS " + COMMCSNAME + " finished" );

//insert data
try
{
	varCL.insert( { a: 3, b: 3, c: "test multi-columns sort" } );
	varCL.insert( { a: 1, b: 1, c: "sequoiadb soft com" } );
	varCL.insert( { a: 4, b: 4, c: "date" } );
	varCL.insert( { a: 0, b: 1, c: "agree sunday" } );
	varCL.insert( { a: 5, b: 4, c: "timestamp" } );
	varCL.insert( { a: 2, b: 1, c: "work-day" } );
} catch( e )
{
	println( "int/bigint insert data failed!" );
	throw e;
}

//query1
//select a,b,c from foo.bar order by b,c
try
{
	var sel = varCL.find( null, { a: null, b: 'b', c: 'c' } ).sort( { b: 1, c: 1 } );
	var flag = true;
	var i = 0;
	while( sel.next() )
	{
		var ret = sel.current();
		if( ret.toObj()['a'] != i )
		{
			flag = false;
			throw "query1-result-uncorrect";
		}
		i++;
		if( i > rownum )
		{
			break;
		}
	}
	sel.close();
	if( flag && i != rownum )
	{
		flag = false;
		throw "query1-result-uncorrect";
	}
} catch( e )
{
	if( e != "query1-result-uncorrect" )
	{
		println( "'select a,b,c from foo.bar order by b,c' failed! rc=" + e );
		throw e;
	} else
	{
		println( "'select a,b,c from foo.bar order by b,c' verify record fail!" );
		throw e;
	}
}
println( "'select a,b,c from foo.bar order by b,c' finished!" );

//create index
try
{
	varCL.createIndex( CLINDEX1, { b: 1 } );
} catch( e )
{
	println( "create indexes fail" );
	throw e;
}
println( "create indexes finished!" );

//query2
//select a,b from foo.bar order by b,c
try
{
	var sel = varCL.find( null, { a: null, b: 'b' } ).sort( { b: 1, c: 1 } ).hint( { "": CLINDEX1 } );
	var flag = true;
	var i = 0;
	while( sel.next() )
	{
		var ret = sel.current();
		if( ret.toObj()['a'] != i )
		{
			flag = false;
			throw "query2-result-uncorrect";
		}
		i++;
		if( i > rownum )
		{
			break;
		}
	}
	sel.close();
	if( flag && i != rownum )
	{
		flag = false;
		throw "query2-result-uncorrect";
	}
} catch( e )
{
	if( e != "query2-result-uncorrect" )
	{
		println( "'select a,b from foo.bar order by b,c' with index failed! rc=" + e );
		throw e;
	} else
	{
		println( "'select a,b from foo.bar order by b,c' with index verify record fail!" );
		throw e;
	}
}
println( "'select a,b from foo.bar order by b,c' with index finished!" );

try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
}
catch( e )
{
	println( "failed to drop cs, rc= " + e );
	throw e;
}

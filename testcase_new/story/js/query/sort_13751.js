/******************************************************************************
@Description : 1. null sort
@Modify list :
               2015-01-17 pusheng Ding  Init
******************************************************************************/
CLINDEX1 = CHANGEDPREFIX + "IND1";
rownum = 4;

/////////////////////query_verify begin/////////////////////////
function query_verify ()
{
	//query1
	//select a,b from foo.bar order by b
	try
	{
		var sel = varCL.find( null, { a: null, b: 'b' } ).sort( { b: 1 } );
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
			println( "'select a,b from foo.bar order by b' failed! rc=" + e );
			throw e;
		} else
		{
			println( "'select a,b from foo.bar order by b' verify record fail!" );
			throw e;
		}
	}
	println( "'select a,b,type from foo.bar order by b' finished!" );

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
	//select a,b from foo.bar order by b desc
	try
	{
		var sel = varCL.find( null, { a: null, b: 'b' } ).sort( { b: -1 } ).hint( { "": CLINDEX1 } );
		var flag = true;
		var i = rownum;
		while( sel.next() )
		{
			i--;
			var ret = sel.current();
			if( ret.toObj()['a'] != i )
			{
				flag = false;
				throw "query2-result-uncorrect";
			}
			if( i < 0 )
			{
				break;
			}
		}
		sel.close();
		if( flag && i != 0 )
		{
			flag = false;
			throw "query2-result-uncorrect";
		}
	} catch( e )
	{
		if( e != "query2-result-uncorrect" )
		{
			println( "'select a,b from foo.bar order by b desc' with index failed! rc=" + e );
			throw e;
		} else
		{
			println( "'select a,b from foo.bar order by b desc' with index verify record fail!" );
			throw e;
		}
	}
	println( "'select a,b,type from foo.bar order by b desc' with index finished!" );
}
/////////////////////query_verify end/////////////////////////

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

/////////////////////////////////int/bigint/////////////////////////////////////////
println( "" );
println( "********************************************************" );
try
{
	varCL.dropIndex( CLINDEX1 );
	varCL.remove();
} catch( e )
{

}
//insert data
try
{
	varCL.insert( { a: 3, b: 3 } );
	varCL.insert( { a: 1, b: 1 } );
	varCL.insert( { a: 0, b: null } );
	varCL.insert( { a: 2, b: 2 } );
} catch( e )
{
	println( "int/bigint insert data failed!" );
	throw e;
}
println( "int/bigint insert data finished!" );
println( "now order-verify begin..." );
query_verify();
println( "now order-verify end" );

/////////////////////////////////float/////////////////////////////////////////
println( "" );
println( "********************************************************" );
try
{
	varCL.dropIndex( CLINDEX1 );
	varCL.remove();
} catch( e )
{

}
//insert data
try
{
	varCL.insert( { a: 0, b: null } );
	varCL.insert( { a: 3, b: 3.12e+10 } );
	varCL.insert( { a: 1, b: -2.0e+10 } );
	varCL.insert( { a: 2, b: 1 } );
} catch( e )
{
	println( "float insert data failed!" );
	throw e;
}
println( "float insert data finished!" );
println( "now order-verify begin..." );
query_verify();
println( "now order-verify end" );

/////////////////////////////////string/////////////////////////////////////////
println( "" );
println( "********************************************************" );
try
{
	varCL.dropIndex( CLINDEX1 );
	varCL.remove();
} catch( e )
{

}
//insert data
try
{
	varCL.insert( { a: 3, b: "sequoiadb " } );
	varCL.insert( { a: 1, b: "hello word" } );
	varCL.insert( { a: 2, b: "nba sports" } );
	varCL.insert( { a: 0, b: null } );
} catch( e )
{
	println( "string insert data failed!" );
	throw e;
}
println( "string insert data finished!" );
println( "now order-verify begin..." );
query_verify();
println( "now order-verify end" );

/////////////////////////////////date/////////////////////////////////////////
println( "" );
println( "********************************************************" );
try
{
	varCL.dropIndex( CLINDEX1 );
	varCL.remove();
} catch( e )
{

}
//insert data
try
{
	varCL.insert( { a: 3, b: { "$date": "2015-08-01" } } );
	varCL.insert( { a: 1, b: { "$date": "2000-12-31" } } );
	varCL.insert( { a: 0, b: null } );
	varCL.insert( { a: 2, b: { "$date": "2010-05-30" } } );
} catch( e )
{
	println( "date insert data failed!" );
	throw e;
}
println( "date insert data finished!" );
println( "now order-verify begin..." );
query_verify();
println( "now order-verify end" );


/////////////////////////////////timestamp/////////////////////////////////////////
println( "" );
println( "********************************************************" );
try
{
	varCL.dropIndex( CLINDEX1 );
	varCL.remove();
} catch( e )
{

}
//insert data
try
{
	varCL.insert( { a: 3, b: { "$timestamp": "2015-01-14-08.30.17.000000" } } );
	varCL.insert( { a: 0, b: null } );
	varCL.insert( { a: 1, b: { "$timestamp": "2011-01-17-14.17.20.123456" } } );
	varCL.insert( { a: 2, b: { "$timestamp": "2011-01-17-14.17.20.567890" } } );
} catch( e )
{
	println( "timestamp insert data failed!" );
	throw e;
}
println( "timestamp insert data finished!" );
println( "now order-verify begin..." );
query_verify();
println( "now order-verify end" );

try
{
	commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
}
catch( e )
{
	println( "failed to drop cs, rc= " + e );
	throw e;
}

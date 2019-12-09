/* *****************************************************************************
@discretion: jira-510: $size
												
@modify list:
   						2014-02-05 Pusheng Ding  Init
***************************************************************************** */

COMMCSNAME = CHANGEDPREFIX + "foo";
COMMCLNAME = CHANGEDPREFIX + "bar";
CHANGEDPREFIX_IDX = COMMCLNAME + "_idx";

try
{

} catch( e )
{
	println( "can't connect to db" );
	throw e;
}

//create CS
try
{
	var varCS = db.createCS( COMMCSNAME );
	println( "createCS " + COMMCSNAME + " finished" );
} catch( e )
{
	//collection space already exist,use it
	if( e != -33 )
	{
		println( "can't create CS:" + COMMCSNAME + " rc=" + e );
		throw e;
	}
	else
	{
		varCS = db.getCS( COMMCSNAME );
		println( "use CS:" + COMMCSNAME );
	}
}

//create CL
try
{
	var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0 } );
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

//create index
try
{
	varCL.createIndex( CHANGEDPREFIX_IDX, { "a": 1 } );
} catch( e )
{
	if( e != -247 )
	{
		println( "create index failed! rc=" + e );
		throw e;
	} else
	{
		println( CHANGEDPREFIX_IDX + " index exists, use it" );
	}
}
println( "create index finished!" );

//insert data
try
{
	varCL.insert( { a: [1, 2, 3, 4], b: 1 } );
	varCL.insert( { a: 1, b: 2 } );
	varCL.insert( { a: [5, 6], b: 3 } );
	varCL.insert( { a: [1, 2], b: 4 } );
} catch( e )
{
	println( "insert data failed! rc=" + e );
	throw e;
}
println( "insert data finished!" );

//find({a:{$size:1,$et:2,$in:[1,4]}})
//result:
//		{a:[1,2],b:4}
println( "********************************************" );
try
{
	var sel1 = varCL.find( { a: { $size: 1, $et: 2, $in: [1, 4] } } ).hint( { "": CHANGEDPREFIX_IDX } );
} catch( e )
{
	println( "find({a:{$size:1,$et:2,$in:[1,4]}}) failed! rc=" + e );
	throw e;
}
//verify data
try
{
	sel1.next();
	var rec1 = sel1.current();
	if( rec1.toObj()['b'] != 4 )
	{
		println( "the 1st record is not expected!" );
		println( "expect:" + "{a:[1,2],b:4}" );
		println( "return:" + rec1 );
		throw 'result1-error';
	}
	sel1.next();
	if( sel1.size() != 0 )
	{
		println( "records are more than expected!" );
		throw "result1-number-error";
	}
	sel1.close();
} catch( e )
{
	if( e == "result1-error" )
	{
		println( "return result:" + sel1 );
	}
	throw e;
}
println( "find({a:{$size:1,$et:2,$in:[1,4]}}) succ!" );

//clean env
try
{
	varCL.dropIndex( CHANGEDPREFIX_IDX );
} catch( e )
{
	println( "drop index failed!rc=" + e );
	throw e;
}

/* *****************************************************************************
@discretion: jira-510: $in $nin $all
												
@modify list:
   						2014-02-04 Pusheng Ding  Init
***************************************************************************** */
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
	varCL.insert( { a: 1, b: 2, c: 'finish' } );
	varCL.insert( { a: -100, b: 4, c: 'over' } );
	varCL.insert( { a: 0, b: 3, c: 'over' } );
	varCL.insert( { a: 100, b: -1, c: 'start' } );
} catch( e )
{
	println( "insert data failed! rc=" + e );
	throw e;
}
println( "insert data finished!" );

//find({a:{$gt:-1,$in:[1,2]}})
//result:
//		{a:1,b:2,c:'finish'}
println( "********************************************" );
try
{
	var sel1 = varCL.find( { a: { $gt: -1, $in: [1, 2] } } ).hint( { "": CHANGEDPREFIX_IDX } );
} catch( e )
{
	println( "find({a:{$gt:-1,$in:[1,2]}}) failed! rc=" + e );
	throw e;
}
//verify data
try
{
	sel1.next();
	var rec1 = sel1.current();
	if( rec1.toObj()['b'] != 2 )
	{
		println( "the 1st record is not expected!" );
		println( "expect:" + "{a:1,b:2,c:'finish'}" );
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
println( "find({a:{$gt:-1,$in:[1,2]}}) succ!" );

//find({a:{$gt:-1,$nin:[1,2]}})
//result:
//		{a:0,b:3,c:'over'}
//		{a:100,b:-1,c:'start'}
println( "********************************************" );
try
{
	var sel2 = varCL.find( { a: { $gt: -1, $nin: [1, 2] } } ).sort( { a: 1 } ).hint( { "": CHANGEDPREFIX_IDX } );
} catch( e )
{
	println( "find({a:{$gt:-1,$nin:[1,2]}}) failed! rc=" + e );
	throw e;
}
//verify data
try
{
	sel2.next();
	var rec1 = sel2.current();
	if( rec1.toObj()['a'] != 0 )
	{
		println( "the 1st record is not expected!" );
		println( "expect:" + "{a:0,b:3,c:'over'}" );
		println( "return:" + rec1 );
		throw 'result2-error';
	}
	sel2.next();
	var rec2 = sel2.current();
	if( rec2.toObj()['a'] != 100 )
	{
		println( "the 2nd record is not expected!" );
		println( "expect:" + "{a:100,b:-1,c:'start'}" );
		println( "return:" + rec2 );
		throw 'result2-error';
	}
	sel2.next();
	if( sel2.size() != 0 )
	{
		println( "records are more than expected!" );
		throw "result2-number-error";
	}
	sel2.close();
} catch( e )
{
	if( e == "result2-error" )
	{
		println( "return result:" + sel2 );
	}
	throw e;
}
println( "find({a:{$gt:-1,$nin:[1,2]}}) succ!" );


//find({a:{$gt:-1,$all:[0,1,100]}})
//result:
//		empty result set
println( "********************************************" );
try
{
	var sel3 = varCL.find( { a: { $gt: -1, $all: [0, 1, 100] } } ).hint( { "": CHANGEDPREFIX_IDX } );
} catch( e )
{
	println( "find({a:{$gt:-1,$all:[0,1,100]}}) failed! rc=" + e );
	throw e;
}
//verify data
try
{
	if( sel3.size() != 0 )
	{
		println( "records are more than expected!" );
		throw "result3-number-error";
	}
	sel3.close();
} catch( e )
{
	if( e == "result3-error" )
	{
		println( "return result:" + sel3 );
	}
	throw e;
}
println( "find({a:{$gt:-1,$all:[0,1,100]}}) succ!" );

//clean env
try
{
	varCL.dropIndex( CHANGEDPREFIX_IDX );
	commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
} catch( e )
{
	println( "drop index failed!rc=" + e );
	throw e;
}

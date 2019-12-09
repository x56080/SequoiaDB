/* *****************************************************************************
@discretion: jira-510: $or/$not with index
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
	varCL.insert( { a: 1, b: 2, c: true } );
	varCL.insert( { a: 1, b: 0, c: "the 2nd record" } );
	varCL.insert( { a: -1, b: 100, c: "large number" } );
	varCL.insert( { b: 101, c: "" } );
} catch( e )
{
	println( "insert data failed! rc=" + e );
	throw e;
}
println( "insert data finished!" );

//find({$or:[{a:1},{a:{$exists:0}}]})
//result:
//		{a:1,b:0,c:"the 2nd record"}
//		{a:1,b:2,c:true}
//		{b:101,c:""}
println( "********************************************" );
try
{
	var sel1 = varCL.find( { $or: [{ a: 1 }, { a: { $exists: 0 } }] } ).sort( { b: 1 } ).hint( { "": CHANGEDPREFIX_IDX } );
} catch( e )
{
	println( "find({$or:[{a:1},{a:{$exists:0}}]}) failed! rc=" + e );
	throw e;
}
//verify data
try
{
	sel1.next();
	var rec1 = sel1.current();
	if( rec1.toObj()['b'] != 0 )
	{
		println( "the 1st record is not expected!" );
		println( "expect:" + "{a:1,b:0,c:'the 2nd record'}" );
		println( "return:" + rec1 );
		throw 'result1-error';
	}
	sel1.next();
	var rec2 = sel1.current();
	if( rec2.toObj()['b'] != 2 )
	{
		println( "the 2nd record is not expected!" );
		println( "expect:" + "{a:1,b:2,c:true}" );
		println( "return:" + rec2 );
		throw 'result1-error';
	}
	sel1.next();
	var rec3 = sel1.current();
	if( rec3.toObj()['b'] != 101 )
	{
		println( "the 3rd record is not expected!" );
		println( "expect:" + "{b:101,c:''}" );
		println( "return:" + rec3 );
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
println( "find({$or:[{a:1},{a:{$exists:0}}]}) succ!" );

//find({$or:[{a:{$gt:0}},{b:{$lt:2}}]})
//result:
//		{a:1,b:0,c:"the 2nd record"}
//		{a:1,b:2,c:true}
println( "********************************************" );
try
{
	var sel2 = varCL.find( { $or: [{ a: { $gt: 0 } }, { b: { $lt: 2 } }] } ).sort( { b: 1 } ).hint( { "": CHANGEDPREFIX_IDX } );
} catch( e )
{
	println( "find({$or:[{a:{$gt:0}},{b:{$lt:2}}]}) failed! rc=" + e );
	throw e;
}
//verify data
try
{
	sel2.next();
	var rec1 = sel2.current();
	if( rec1.toObj()['b'] != 0 )
	{
		println( "the 1st record is not expected!" );
		println( "expect:" + "{a:1,b:0,c:'the 2nd record'}" );
		println( "return:" + rec1 );
		throw 'result2-error';
	}
	sel2.next();
	var rec2 = sel2.current();
	if( rec2.toObj()['b'] != 2 )
	{
		println( "the 2nd record is not expected!" );
		println( "expect:" + "{a:1,b:2,c:true}" );
		println( "return:" + rec2 );
		throw 'result1-error';
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
		println( "return result:" + sel1 );
	}
	throw e;
}
println( "find({$or:[{a:{$gt:0}},{b:{$lt:2}}]}) succ!" );

//find({$not:[{a:{$ne:1}},{b:{$lt:101}}]})
//result:
//		{a:1,b:0,c:"the 2nd record"}
//		{a:1,b:2,c:true}
//		{b:101,c:""}
println( "********************************************" );
try
{
	var sel3 = varCL.find( { $not: [{ a: { $ne: 1 } }, { b: { $lt: 101 } }] } ).sort( { b: 1 } ).hint( { "": CHANGEDPREFIX_IDX } );
} catch( e )
{
	println( "find({$not:[{a:{$ne:1}},{b:{$lt:101}}]}) failed! rc=" + e );
	throw e;
}
//verify data
try
{
	sel3.next();
	var rec1 = sel3.current();
	if( rec1.toObj()['b'] != 0 )
	{
		println( "the 1st record is not expected!" );
		println( "expect:" + "{a:1,b:0,c:'the 2nd record'}" );
		println( "return:" + rec1 );
		throw 'result1-error';
	}
	sel3.next();
	var rec2 = sel3.current();
	if( rec2.toObj()['b'] != 2 )
	{
		println( "the 2nd record is not expected!" );
		println( "expect:" + "{a:1,b:2,c:true}" );
		println( "return:" + rec2 );
		throw 'result1-error';
	}
	sel3.next();
	var rec3 = sel3.current();
	if( rec3.toObj()['b'] != 101 )
	{
		println( "the 3rd record is not expected!" );
		println( "expect:" + "{b:101,c:''}" );
		println( "return:" + rec3 );
		throw 'result1-error';
	}
	sel3.next();
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
println( "find({$not:[{a:{$ne:1}},{b:{$lt:101}}]}) succ!" );

//clean env
try
{
	varCL.dropIndex( CHANGEDPREFIX_IDX );
} catch( e )
{
	println( "drop index failed!rc=" + e );
	throw e;
}

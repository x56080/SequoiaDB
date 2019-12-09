/****************************************************
@description: 	对数据压缩，集合中不创建_id索引，
					用bulkInsert方式插入数据，对多条记录进行增删改查
@author:
             	2015-10-21 TingYU
****************************************************/
main();

function main ()
{
	try
	{
		var cl = readyCL( { ReplSize: 0, AutoIndexId: false, Compressed: true } );

		var expRecs = [];

		insertRecs( cl, expRecs );
		queryRecs( cl, expRecs );
		upsertRecs( cl );
		updateRecs( cl );
		//		findAndRemoveRecs( cl );  //has bug, jira1323
		//		findAndUpdateRecs( cl );
		removeRecs( cl );
		queryRecs( cl, expRecs );
		truncateCL( cl, expRecs );

		clean();
	}
	catch( e )
	{
		throw e;
	}
	finally
	{
	}
}

function insertRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	for( var i = 0; i < 1000; i++ )
	{
		var rec = { a: i, b: i, c: i };
		expRecs.push( rec );
	}
	cl.insert( expRecs );

	var rc = cl.find();
	checkRec( rc, expRecs );
}

function queryRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	//find with sort/limit/skip/size
	var rc = cl.find().sort( { a: -1 } ).limit( 20 ).skip( 10 );

	var tmpExpRecs = expRecs.concat();
	tmpExpRecs.reverse();
	tmpExpRecs = tmpExpRecs.slice( 10, 10 + 20 );
	checkRec( rc, tmpExpRecs );

	//find with limit/skip/size
	var cnt = cl.find().limit( 180 ).skip( 900 ).size();
	if( parseInt( cnt ) !== 100 )
	{
		throw buildException( "find with size", null, "cl.find().limit(180).skip(900).size()", 100, cnt );
	}

	//find with count
	var cnt = cl.find( { a: { $ne: 0 } } ).count();
	if( parseInt( cnt ) !== 999 )
	{
		throw buildException( "find with count", null, "cl.find({a:{$ne:0}}).count()", 999, cnt );
	}
}

function upsertRecs ( cl )
{
	println( "\n---begin to excute " + getFuncName() );

	try
	{
		cl.upsert( { $set: { a: -2 } }, { a: 0 } );
		var throwErr279 = false;
	}
	catch( e )
	{
		if( e === -279 )
		{
			throwErr279 = true;
		}
		else
		{
			throw buildException( "", null, "upsert", "e=-279", "e=" + e );
		}
	}
	if( throwErr279 === false )
	{
		throw buildException( "", null, "upsert", "e=-279", "did not throw error" );
	}
}

function updateRecs ( cl )
{
	println( "\n---begin to excute " + getFuncName() );

	try
	{
		cl.update( { $inc: { a: 1 } } );
		var throwErr279 = false;
	}
	catch( e )
	{
		if( e === -279 )
		{
			throwErr279 = true;
		}
		else
		{
			throw buildException( "", null, "update", "e=-279", "e=" + e );
		}
	}
	if( throwErr279 === false )
	{
		throw buildException( "", null, "update", "e=-279", "did not throw error" );
	}
}

function findAndRemoveRecs ( cl )
{
	println( "\n---begin to excute " + getFuncName() );

	try
	{
		cl.find( { a: 1 } ).remove();
		var throwErr279 = false;
	}
	catch( e )
	{
		if( e === -279 )
		{
			throwErr279 = true;
		}
		else
		{
			throw buildException( "", null, "findAndRemove", "e=-279", "e=" + e );
		}
	}
	if( throwErr279 === false )
	{
		throw buildException( "", null, "findAndRemove", "e=-279", "did not throw error" );
	}
}

function findAndUpdateRecs ( cl )
{
	println( "\n---begin to excute " + getFuncName() );

	try
	{
		cl.find( { b: { $gte: 10000 } } ).update( { $inc: { b: -1 } } )
		var throwErr279 = false;
	}
	catch( e )
	{
		if( e === -279 )
		{
			throwErr279 = true;
		}
		else
		{
			throw buildException( "", null, "findAndUpdate", "e=-279", "e=" + e );
		}
	}
	if( throwErr279 === false )
	{
		throw buildException( "", null, "findAndUpdate", "e=-279", "did not throw error" );
	}
}

function removeRecs ( cl )
{
	println( "\n---begin to excute " + getFuncName() );

	try
	{
		cl.remove();
		var throwErr279 = false;
	}
	catch( e )
	{
		if( e === -279 )
		{
			throwErr279 = true;
		}
		else
		{
			throw buildException( "", null, "remove", "e=-279", "e=" + e );
		}
	}
	if( throwErr279 === false )
	{
		throw buildException( "", null, "remove", "e=-279", "did not throw error" );
	}
}

function truncateCL ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.truncate();

	expRecs.splice( 0, expRecs.length );//remove all elements
	var rc = cl.find();
	checkRec( rc, expRecs );
}
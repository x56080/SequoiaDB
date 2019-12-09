/****************************************************
@description: 对数据压缩，对多条记录进行增删改查等操作
@author:
              2015-10-21 TingYU
****************************************************/
main();

function main ()
{
	try
	{
		var cl = readyCL( { ReplSize: 0, Compressed: true } );

		var expRecs = [];
		upsertRecs( cl, expRecs );
		updateRecs( cl, expRecs );
		insertRecs( cl, expRecs );
		queryRecs( cl, expRecs );
		findAndUpdateRecs( cl, expRecs );
		findAndRemoveRecs( cl, expRecs );
		removeRecs( cl, expRecs );
		truncateCL( cl, expRecs );
		upsertRecs( cl, expRecs );

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

function upsertRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	for( var i = 0; i < 1000; i++ )
	{
		var rec = { a: i };
		cl.upsert( { $set: rec }, { a: -1 } );
		expRecs.push( rec );
	}

	var rc = cl.find();
	checkRec( rc, expRecs );
}

function updateRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.update( { $inc: { a: -1 } }, { a: { $lt: 100 } } );

	for( var i = 0; i < 100; i++ )
	{
		expRecs[i]["a"] -= 1;
	}
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function insertRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	for( var i = 1000; i < 2000; i++ )
	{
		var rec = { a: i };
		cl.insert( rec );
		expRecs.push( rec );
	}

	var rc = cl.find().sort( { a: 1 } );
	checkRec( rc, expRecs );
}

function queryRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );
	//find with sort/limit/skip
	var rc = cl.find().sort( { a: -1 } ).limit( 20 ).skip( 10 );

	var tmpExpRecs = expRecs.concat(); //copy array
	tmpExpRecs.reverse();
	tmpExpRecs = tmpExpRecs.slice( 10, 10 + 20 );//return a part of array
	checkRec( rc, tmpExpRecs );

	//find with count
	var cnt = cl.find( { a: { $ne: 0 } } ).count();
	if( parseInt( cnt ) !== 1999 )
	{
		throw buildException( "", null, "cl.find({a:{$ne:0}}).count()", 1999, cnt );
	}
}

function findAndUpdateRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	var rcFU = cl.find( { a: { $lt: 100 } } ).update( { $inc: { a: 1 } } );
	while( rcFU.next() );  //cursor need to be traversed

	for( var i = 0; i < 100; i++ )
	{
		expRecs[i]["a"] += 1;
	}
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function findAndRemoveRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	var rcFR = cl.find( { a: { $et: 200 } } ).remove();
	while( rcFR.next() );  //cursor need to be traversed

	expRecs.splice( 200, 1 );
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function removeRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.remove( { a: 20 } );

	expRecs.splice( 20, 1 );
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function truncateCL ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.truncate();

	expRecs.splice( 0, expRecs.length );//remove all elements in array
	var rc = cl.find();
	checkRec( rc, expRecs );
}
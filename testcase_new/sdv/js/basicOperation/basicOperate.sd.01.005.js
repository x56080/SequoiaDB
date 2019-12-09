/****************************************************
@description: 	创建多个索引，创建联合索引，
					用bulkInsert方式插入数据，对多条记录进行增删改查
@author:
					2015-10-21 TingYU
****************************************************/
main();

function main ()
{
	try
	{
		var cl = readyCL( { ReplSize: 0 } );
		var idxName1 = 'aIdx';
		commCreateIndex( cl, idxName1, { a: -1 }, false, false );
		var idxName2 = 'bcIdx';
		commCreateIndex( cl, idxName2, { b: 1, c: -1 }, true, false );

		var expRecs = [];

		insertRecs( cl, expRecs );
		removeRecs( cl, expRecs );
		upsertRecs( cl, expRecs );
		queryRecs( cl, expRecs, idxName1, idxName2 )
		updateRecs( cl, expRecs );
		findAndUpdateRecs( cl, expRecs );
		findAndRemoveRecs( cl, expRecs );
		truncateCL( cl, expRecs );
		insertRecs( cl, expRecs );

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

function removeRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.remove();

	expRecs.splice( 0, expRecs.length );//remove all elements in array
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function upsertRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	for( var i = 0; i < 1000; i++ )
	{
		var rec = { a: i, b: i, c: i };
		cl.upsert( { $set: rec }, { b: i % 500 } );
	}

	for( var i = 500; i < 1000; i++ )
	{
		var rec = { a: i, b: i, c: i };
		expRecs.push( rec );
	}
	var rc = cl.find().sort( { a: 1 } );
	checkRec( rc, expRecs );
}

function queryRecs ( cl, expRecs, idxName1, idxName2 )
{
	println( "\n---begin to excute " + getFuncName() );

	//find with sort/limit/skip/explain by sigle index
	var rc = cl.find().sort( { a: 1 } ).limit( 20 ).skip( 10 );
	checkRec( rc, expRecs.slice( 10, 10 + 20 ) );
	checkExplain( rc, idxName1 );

	//find with sort/limit/skip by combined index
	var rc = cl.find().sort( { b: -1, c: 1 } ).limit( 2 ).skip( 100 );
	var tmpExpRecs = expRecs.concat();
	tmpExpRecs.reverse();
	tmpExpRecs = tmpExpRecs.slice( 100, 100 + 2 );
	checkExplain( rc, idxName2 );

	//find with sort/hint by sigle index
	var rc = cl.find().sort( { b: 1 } ).hint( { "": idxName1 } );
	checkRec( rc, expRecs );
	checkExplain( rc, idxName1 );
}

function updateRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.update( { $inc: { a: 1 } } );

	for( var i in expRecs )
	{
		expRecs[i]["a"] += 1;
	}
	var rc = cl.find().sort( { b: 1 } );
	checkRec( rc, expRecs );
}

function findAndUpdateRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	var rcFU = cl.find( { b: { $gte: 10000 } } ).update( { $inc: { b: -1 } } );
	while( rcFU.next() );

	var rc = cl.find().sort( { c: 1 } );
	checkRec( rc, expRecs );
}

function findAndRemoveRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	var rcFR = cl.find().remove();
	while( rcFR.next() );

	expRecs.splice( 0, expRecs.length );//remove all elements in array
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


/****************************************************
@description: 对数据压缩，创建索引，对单条记录进行增删改查等操作
@author:
              2015-10-21 TingYU
****************************************************/
main();

function main ()
{
	try
	{
		var cl = readyCL( { ReplSize: 0, Compressed: true } );
		var idxName = 'aIdx';
		commCreateIndex( cl, idxName, { a: 1 }, false, false );

		var expRecs = [];
		insertRecs( cl, expRecs );
		queryRecs( cl, expRecs, idxName );
		updateRecs( cl, expRecs );
		findAndRemoveRecs( cl, expRecs );
		upsertRecs( cl, expRecs );
		findAndUpdateRecs( cl, expRecs );
		removeRecs( cl, expRecs );
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

	expRecs.push( { _id: 1, a: 1, b: 1 } );
	cl.insert( expRecs );

	var rc = cl.find();
	checkRec( rc, expRecs );
}

function queryRecs ( cl, expRecs, idxName )
{
	println( "\n---begin to excute " + getFuncName() );

	var rc = cl.find( { a: 1 } );
	checkRec( rc, expRecs );
	checkExplain( rc, idxName );

	var rc = cl.find( { a: 0 } );
	checkRec( rc, [] );
	checkExplain( rc, idxName );
}

function updateRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.update( { $inc: { a: 1 } } );

	expRecs[0]["a"] += 1;
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function findAndRemoveRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );
	var rcFR = cl.find( { a: { $lt: 10 } } ).remove();
	while( rcFR.next() );  //cursor need to be traversed

	expRecs.pop();
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function upsertRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.upsert( { $set: { a: "str", b: 3 } }, { a: { $exists: 1 } } );

	expRecs.push( { a: "str", b: 3 } );
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function findAndUpdateRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	var rcFU = cl.find( { a: { $type: 1, $et: 2 } } ).update( { $inc: { b: 100 } } );
	while( rcFU.next() );  //cursor need to be traversed

	expRecs[0]["b"] = 103;;
	var rc = cl.find();
	checkRec( rc, expRecs );
}

function removeRecs ( cl, expRecs )
{
	println( "\n---begin to excute " + getFuncName() );

	cl.remove( { a: null } );//remove non-existed record

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



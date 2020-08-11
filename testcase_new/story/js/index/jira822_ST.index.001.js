/****************************************************
@description: seqDB-6079:创建{a.b:1}索引_ST.index.001 
@author:
              2015-11-9 TingYU
****************************************************/
main();

function main ()
{
	try
	{
		var csName = COMMCSNAME;
		var clName = COMMCLNAME;
		var cl = readyCL( csName, clName );

		println( "\n---begin to insert records" );
		cl.insert( { a: [{ b: 0 }, { c: 1 }] } );
		cl.insert( { a: [{ b: 2 }, { c: 3 }] } );
		cl.insert( { a: [5, 6, 7, 8, 9] } );

		println( "\n---begin to create index" );
		var idxName = 'aIdx';
		commCreateIndex( cl, idxName, { "a.b": -1 } );

		println( "\n---begin to query by index" );
		var rc = cl.find( { "a.$1.b": 2 } );
		var expRecs = [{ a: [{ b: 2 }, { c: 3 }] }];
		checkRec( rc, expRecs );
		checkExplain( rc, idxName );

		clean( csName, clName );
	}
	catch( e )
	{
		throw e;
	}
	finally
	{
	}
}

function readyCL ( csName, clName, option )
{
	println( "\n---begin to create cl" );

	commDropCL( db, csName, clName, true, true, "drop cl in begin" );
	cl = commCreateCL( db, csName, clName, option, true, false, "create cl in begin" );

	return cl;
}

function checkRec ( rc, expRecs )
{
	//get actual records to array
	var actRecs = [];
	while( rc.next() )
	{
		actRecs.push( rc.current().toObj() );
	}

	//check count
	if( actRecs.length !== expRecs.length )
	{
		println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
		throw buildException( "check count", null, "",
			expRecs.length, actRecs.length );
	}

	//check every records every fields
	for( var i in expRecs )
	{
		var actRec = actRecs[i];
		var expRec = expRecs[i];
		for( var f in expRec )
		{
			if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
			{
				println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
				println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
				throw buildException( "checkRec()", "rec ERROR" );
			}
		}
	}
}

function checkExplain ( rc, idxName )
{
	var plan = rc.explain().current().toObj();
	if( ( plan.ScanType === "ixscan" ) && ( plan.IndexName === idxName ) )
	{	//ok
	}
	else
	{
		throw buildException( "checkExplain()", null, "query.explain()",
			"ScanType:ixscan, IndexName:" + idxName,
			"ScanType:" + plan.ScanType + ", IndexName:" + plan.IndexName );
	}
}


function clean ( csName, clName )
{
	println( "\n---begin to clean environment" );

	commDropCL( db, csName, clName, true, true, "drop cl in clean" );
}



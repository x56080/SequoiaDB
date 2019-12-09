/************************************
 *@Description: 测试用例 seqDB-12186 :: 版本: 1 :: 开启flag指定匹配符和更新符更新分区键 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12186";
var clName = CHANGEDPREFIX + "_cl_12186";

function main ()
{
	if( true == commIsStandalone( db ) )
	{
		println( "run mode is standalone" );
		return;
	}

	var allGroupName = getGroupName( db, true );
	if( 1 === allGroupName.length )
	{
		println( "--least two groups" );
		return;
	}

	commDropCS( db, csName, true, "Failed to drop CS." );
	commCreateCS( db, csName, false, "Failed to create CS." );
	var mycl = createCL( csName, clName, { "a": 1 } );

	var matcheCondition = [
		{ a: { $et: 1 } },
		{ a: { $isnull: 0 } },
		{ a: { $in: [1, 2] } },
		{ a: { $nin: [3, 4] } },
		{ a: { $exists: 1 } }
	];

	var notMatcheCondition = [
		{ a: { $et: 0 } },
		{ a: { $isnull: 1 } },
		{ a: { $nin: [1, 2] } },
		{ a: { $in: [3, 4] } },
		{ a: { $exists: 0 } }
	];

	for( i in matcheCondition )
	{
		mycl.remove();
		mycl.insert( { a: 1 } );
		println( JSON.stringify( matcheCondition[i] ) );
		mycl.upsert( { $set: { "a": "test" } }, matcheCondition[i], {}, {}, { KeepShardingKey: true } );
		checkResult( mycl, null, null, [{ a: "test" }] );
	}

	mycl.remove();
	mycl.insert( { a: 1 } );
	for( i in notMatcheCondition )
	{
		println( JSON.stringify( notMatcheCondition[i] ) );
		mycl.update( { $set: { "a": "test" } }, notMatcheCondition[i], {}, {}, { KeepShardingKey: true } );
		checkResult( mycl, null, null, [{ a: 1 }] );
	}

}
//main();

/************************************
*@Description: upsert object with pull_by
*@author:      liuxiaoxuan
*@createdate:  2017.09.18
*@testlinkCase: seqDB-12760
**************************************/
function main ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

	//insert object
	var doc = [{ a: 1 }, { a: 'a' }];
	insertData( dbcl, doc );

	//upsert array objects 
	var upsertRule = {
		$pull_by: {
			a1: 100, a2: 'a',
			a3: 1, a4: -1,
			'a5.1': 30, 'a6.1.1': 10,
			'a7.0': 30,
			'a8.1.1': 100,
			a9: { obj1: 3 },
			a10: { obj1: 1 },
			a11: { obj1: 1, obj2: 'testa112' },
			a12: { obj1: { obj2: -1 } },
			a13: { obj1: { obj2: 1 } },
			a14: { obj1: { obj2: { obj3: -1 } } },
			a15: { obj1: { obj2: { obj3: 1 } } }
		}
	};
	//set condition
	var findConditions = [{ a1: { $et: [1, 2, 0, -2, -1] } },
	{ a2: { $et: ['a', 'b', 'c', 'd'] } },
	{ a3: [-1, [0, 1], 2] },
	{ a4: [-1, [0, 1], 2] },
	{ a5: [-10, [0, [10, 20]], 30] },
	{ a6: [-10, [0, [10, 20]], 30] },
	{ a7: { $all: [10, [0, -10, -20], 20] } },
	{ a8: { $all: [0, [-100, [0, 1, 100]], [-200, 200]] } },
	{ a9: [{ obj1: 1, obj2: 'testa91' }, { obj1: 2, obj2: 'testa92' }] },
	{ a10: [{ obj1: 1, obj2: 'testa101' }, { obj1: 1 }, { obj1: 2, obj2: 'testa102' }] },
	{ a11: [{ obj1: 1, obj2: 'testa111' }, { obj1: 2, obj2: 'testa112' }, { obj1: 1 }] },
	{ a12: [{ obj1: { obj2: 1, obj3: 'testa121' } }, { obj1: { obj2: 2, obj3: 'testa122' } }] },
	{ a13: [{ obj1: { obj2: 1, obj3: 'testa131' } }, { obj1: { obj2: 1 } }, { obj1: { obj2: 2, obj3: 'testa132' } }] },
	{ a14: [{ obj1: { obj2: { obj3: 1, obj4: 'testa141' } } }, { obj1: { obj2: { obj3: 2, obj4: 'testa142' } } }] },
	{ a15: [{ obj1: { obj2: { obj3: 1, obj4: 'testa151' } } }, { obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2, obj4: 'testa152' } } }] }, ,
	{ a16: 'testaaaaaaaa16' },
	{ a17: { a18: 1 } }];

	for( var i in findConditions )   
	{
		upsertData( dbcl, upsertRule, findConditions[i] );
	}

	//check result
	var expectResult = [{ a1: [1, 2, 0, -2, -1] },
	{ a2: ['b', 'c', 'd'] },
	{ a3: [-1, [0, 1], 2] },
	{ a4: [[0, 1], 2] },
	{ a5: [-10, [0, [10, 20]], 30] },
	{ a6: [-10, [0, [20]], 30] },
	{ a7: [10, [0, -10, -20], 20] },
	{ a8: [0, [-100, [0, 1]], [-200, 200]] },
	{ a9: [{ obj1: 1, obj2: 'testa91' }, { obj1: 2, obj2: 'testa92' }] },
	{ a10: [{ obj1: 2, obj2: 'testa102' }] },
	{ a11: [{ obj1: 1, obj2: 'testa111' }, { obj1: 2, obj2: 'testa112' }, { obj1: 1 }] },
	{ a12: [{ obj1: { obj2: 1, obj3: 'testa121' } }, { obj1: { obj2: 2, obj3: 'testa122' } }] },
	{ a13: [{ obj1: { obj2: 1, obj3: 'testa131' } }, { obj1: { obj2: 2, obj3: 'testa132' } }] },
	{ a14: [{ obj1: { obj2: { obj3: 1, obj4: 'testa141' } } }, { obj1: { obj2: { obj3: 2, obj4: 'testa142' } } }] },
	{ a15: [{ obj1: { obj2: { obj3: 1, obj4: 'testa151' } } }, { obj1: { obj2: { obj3: 2, obj4: 'testa152' } } }] },
	{ a16: 'testaaaaaaaa16' },
	{ a17: { a18: 1 } },
	{ a: 1 },
	{ a: 'a' }];
	checkResult( dbcl, null, null, expectResult, { a: 1 } );

	//remove data
	dbcl.remove();

	//insert data 
	var doc = [{ a: -1 }, { a: 'test' }];
	insertData( dbcl, doc );

	//without condition,will not insert data
	upsertRule = { $pull_by: { a1: { obj: 1 } } };
	upsertData( dbcl, upsertRule );

	//check result
	expectResult = [{ a: -1 }, { a: 'test' }];
	checkResult( dbcl, null, null, expectResult, { _id: 1 } );
}

try
{
	main();
}
catch( e )
{
	if( e.constructor === Error )
	{
		println( e.stack );
	}
	throw e;
}
;
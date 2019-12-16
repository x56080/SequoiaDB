/************************************
*@Description: upsert object with pull_all_by
*@author:      liuxiaoxuan
*@createdate:  2017.09.19
*@testlinkCase: seqDB-12766
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
		$pull_all_by: {
			a1: [-1, 0, 2], a2: ['b', 'd'],
			a3: [100, 200], a4: [-1, 2],
			'a5.1.1': [100, 200, 300], 'a6.1.1': [10, 20, 30],
			'a7.1.1': [-10, 1, 20, 200],
			a8: [{ obj1: 1 }, { obj1: 2 }],
			a9: [{ obj1: -1 }, { obj2: 'obj2' }],
			a10: [{ obj1: 1, obj2: 'testa101' }, { obj1: 2, obj2: 'aobj2' }],
			a11: [{ obj1: { obj2: 1 } }, { obj1: { obj2: 2 } }],
			a12: [{ obj1: { obj2: 1, obj3: 'testa121' } }, { obj1: { obj2: 0, obj3: 'testa122' } }],
			a13: [{ obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2 } } }],
			a14: [{ obj1: { obj2: { obj3: 1, obj4: 'testa141' } } }, { obj1: { obj2: { obj3: -1 } } }, { obj4: 'aobj4' }]
		}
	};
	//set condition
	var findConditions = [{ a1: { $et: [1, 2, 0, -2, -1] } },
	{ a2: { $et: ['a', 'b', 'c', 'd'] } },
	{ a3: [-1, [0, 1], 2, 3] },
	{ a4: [-1, [0, 1], 2, 3] },
	{ a5: [-10, [0, [10, 20, 30]], 40] },
	{ a6: [-10, [0, [10, 20, 30]], 40] },
	{ a7: { $all: [0, [-100, [0, 1, 100]], [-200, 200]] } },
	{ a8: [{ obj1: 1, obj2: 'testa81' }, { obj1: 2, obj2: 'testa82' }, { obj1: 3 }] },
	{ a9: [{ obj1: 1, obj2: 'testa91' }, { obj1: 2, obj2: 'testa92' }] },
	{ a10: [{ obj1: 1, obj2: 'testa101' }, { obj1: 2, obj2: 'testa102' }] },
	{ a11: [{ obj1: { obj2: 1, obj3: 'testa111' } }, { obj1: { obj2: 2, obj3: 'testa112' } }] },
	{ a12: [{ obj1: { obj2: 0, obj3: 'testa121' } }, { obj1: { obj2: -1, obj3: 'testa122' } }] },
	{ a13: [{ obj1: { obj2: { obj3: 1, obj4: 'testa131' } } }, { obj1: { obj2: { obj3: 2, obj4: 'testa132' } } }] },
	{ a14: [{ obj1: { obj2: { obj3: 1, obj4: 'testa141' } } }, { obj1: { obj2: { obj3: 2, obj4: 'testa142' } } }] },
	{ a15: 'testaaaaaaaa15' },
	{ a16: { a17: 1 } }];

	for( var i in findConditions )   
	{
		upsertData( dbcl, upsertRule, findConditions[i] );
	}


	//check result
	var expectResult = [{ a1: [1, -2] },
	{ a2: ['a', 'c'] },
	{ a3: [-1, [0, 1], 2, 3] },
	{ a4: [[0, 1], 3] },
	{ a5: [-10, [0, [10, 20, 30]], 40] },
	{ a6: [-10, [0, []], 40] },
	{ a7: [0, [-100, [0, 100]], [-200, 200]] },
	{ a8: [{ obj1: 3 }] },
	{ a9: [{ obj1: 1, obj2: 'testa91' }, { obj1: 2, obj2: 'testa92' }] },
	{ a10: [{ obj1: 2, obj2: 'testa102' }] },
	{ a11: [{ obj1: { obj2: 1, obj3: 'testa111' } }, { obj1: { obj2: 2, obj3: 'testa112' } }] },
	{ a12: [{ obj1: { obj2: 0, obj3: 'testa121' } }, { obj1: { obj2: -1, obj3: 'testa122' } }] },
	{ a13: [{ obj1: { obj2: { obj3: 1, obj4: 'testa131' } } }, { obj1: { obj2: { obj3: 2, obj4: 'testa132' } } }] },
	{ a14: [{ obj1: { obj2: { obj3: 2, obj4: 'testa142' } } }] },
	{ a15: 'testaaaaaaaa15' },
	{ a16: { a17: 1 } },
	{ a: 1 },
	{ a: 'a' }];
	checkResult( dbcl, null, null, expectResult, { a: 1 } );

	//remove data
	dbcl.remove();

	//insert data 
	var doc = [{ a: -1 }, { a: 'test' }, { a: [0, 2, 4, 10, 100] }];
	insertData( dbcl, doc );

	//without condition, will not insert data
	upsertRule = { $pull_all_by: { a: [-1, 0, 2, 3, 'a', 'test'] } };
	upsertData( dbcl, upsertRule );

	//check result
	expectResult = [{ a: -1 }, { a: 'test' }, { a: [4, 10, 100] }];
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
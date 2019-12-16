/************************************
*@Description: update object does not exist,use operator push
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
function main ()
{
	//clear environment
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

	//insert data
	var doc1 = [{ object0: [10, -30, 20] }];
	insertData( dbcl, doc1 );

	//update use push object does not exist,no matches
	var updateCondition1 = {
		$push: {
			object1: 10,
			object2: "string",
			"object3.1": 20,
			"object4.1": { $date: "2016-05-16" }
		}
	};
	updateData( dbcl, updateCondition1 );

	//check result
	var expRecs1 = [{
		object0: [10, -30, 20],
		object1: [10],
		object2: ["string"],
		object3: { 1: [20] },
		object4: { 1: [{ $date: "2016-05-16" }] }
	}];
	checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

	//insert data
	var doc2 = [{ object5: [10, -30, 20] }];
	insertData( dbcl, doc2 );

	//update use push object does not exist,with matches
	var updateCondition2 = {
		$push: {
			object1: 10,
			object2: "string",
			"object3.2": 20,
			"object4.1": { $date: "2016-05-16" }
		}
	};
	var findCondition2 = { object1: { $exists: 1 } };
	updateData( dbcl, updateCondition2, findCondition2 );

	//check result
	var expRecs2 = [{
		object0: [10, -30, 20],
		object1: [10, 10],
		object2: ["string", "string"],
		object3: { 1: [20], 2: [20] },
		object4: { 1: [{ $date: "2016-05-16" }, { $date: "2016-05-16" }] }
	},
	{ object5: [10, -30, 20] }];
	checkResult( dbcl, null, null, expRecs2, { _id: 1 } );
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
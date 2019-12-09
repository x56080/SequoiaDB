/************************************
*@Description: update exist object,use operator replace
               replace object's name can not include . and the first letter can not $
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
function main ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

	//insert data   
	var doc1 = [{ object1: 123 },
	{ "object2.0": { $oid: "573920accc332f037c000013" } }];
	insertData( dbcl, doc1 );

	//update use replace exist object,no matches
	var updateCondition1 = {
		$replace: {
			object1: 10,
			object2: { $date: "2016-05-16" }
		}
	};
	updateData( dbcl, updateCondition1 );

	//check result
	var expRecs1 = [{
		object1: 10,
		object2: { $date: "2016-05-16" }
	},
	{
		object1: 10,
		object2: { $date: "2016-05-16" }
	}];
	checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

	//insert data
	var doc2 = [{ object: [10, -30, 20] }];
	insertData( dbcl, doc2 );

	//update use replace exist object,with matches
	var updateCondition2 = {
		$replace: {
			object3: [10, 5, 7],
			object4: { firstName: "han", lastName: "meimei" }
		}
	};
	var findCondition2 = { object: { $exists: 1 } };
	updateData( dbcl, updateCondition2, findCondition2 );

	//check result
	var expRecs2 = [{
		object1: 10,
		object2: { $date: "2016-05-16" }
	},
	{
		object1: 10,
		object2: { $date: "2016-05-16" }
	},
	{
		object3: [10, 5, 7],
		object4: { firstName: "han", lastName: "meimei" }
	}];
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
/************************************
*@Description: update exist object,use operator push
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
	var doc1 = [{ object1: [10, -30, 20] },
	{ object2: 12 },
	{ object3: [10, 30, -20, 15, 99, 80] },
	{ object4: [200, [305, -299, 400], 400] },
	{
		object5: [-2147483640,
		{ $numberLong: "-9223372036854775800" },
		{ $decimal: "9223372036854775800" },
			"string",
		{ $oid: "573920accc332f037c000013" },
			false,
		{ $date: "2016-05-16" },
		{ $timestamp: "2016-05-16-13.14.26.124233" },
		{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
		{ $regex: "^z", $options: "i" },
			null]
	},
	{ object6: [200, [305, -299, 400, 1, 50, 1000], 400] },
	{ object7: [200, [305, -299, [400, 1, 50], 1000], 400] },
	{ object8: [200, [305, -299, 400, 1, 50, 1000], 400] }];
	insertData( dbcl, doc1 );

	//update use push exist object,no matches
	var updateCondition1 = {
		$push: {
			object1: 10,
			object2: 13,
			object3: 100,
			object5: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
			"object6.1": -299,
			"object7.1.2": 10,
			"object8.1.2": 20
		}
	};
	updateData( dbcl, updateCondition1 );

	//check result
	var expRecs1 = [{
		object1: [10, -30, 20, 10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: 12,
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [10, 30, -20, 15, 99, 80, 100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object4: [200, [305, -299, 400], 400],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [-2147483640,
		{ $numberLong: "-9223372036854775800" },
		{ $decimal: "9223372036854775800" },
			"string",
		{ $oid: "573920accc332f037c000013" },
			false,
		{ $date: "2016-05-16" },
		{ $timestamp: "2016-05-16-13.14.26.124233" },
		{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
		{ $regex: "^z", $options: "i" },
			null,
		{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: [200, [305, -299, 400, 1, 50, 1000, -299], 400],
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: [200, [305, -299, [400, 1, 50, 10], 1000], 400],
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: [200, [305, -299, 400, 1, 50, 1000], 400]
	}];
	checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

	//update use push exist object,with matches
	var updateCondition2 = {
		$push: {
			object1: 10,
			object2: 13,
			object3: 100,
			object5: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
			"object6.1": -299,
			"object7.1.2": 10,
			"object8.1.2": 20
		}
	};
	var findCondition2 = { object4: { $exists: 1 } };
	updateData( dbcl, updateCondition2, findCondition2 );

	//check result
	var expRecs2 = [{
		object1: [10, -30, 20, 10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: 12,
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [10, 30, -20, 15, 99, 80, 100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10, 10],
		object2: [13, 13],
		object3: [100, 100],
		object4: [200, [305, -299, 400], 400],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299, -299] },
		object7: { 1: { 2: [10, 10] } },
		object8: { 1: { 2: [20, 20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [-2147483640,
		{ $numberLong: "-9223372036854775800" },
		{ $decimal: "9223372036854775800" },
			"string",
		{ $oid: "573920accc332f037c000013" },
			false,
		{ $date: "2016-05-16" },
		{ $timestamp: "2016-05-16-13.14.26.124233" },
		{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
		{ $regex: "^z", $options: "i" },
			null,
		{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: [200, [305, -299, 400, 1, 50, 1000, -299], 400],
		object7: { 1: { 2: [10] } },
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: [200, [305, -299, [400, 1, 50, 10], 1000], 400],
		object8: { 1: { 2: [20] } }
	},
	{
		object1: [10],
		object2: [13],
		object3: [100],
		object5: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
		object6: { 1: [-299] },
		object7: { 1: { 2: [10] } },
		object8: [200, [305, -299, 400, 1, 50, 1000], 400]
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
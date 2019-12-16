/************************************
*@Description: update any object(exist or not exist) use operator addtoset
*@author:      zhaoyu
*@createdate:  2016.5.17
**************************************/
function main ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

	//insert data   
	var doc1 = [{ object1: [10, 30, 20] },
	{ object2: 12 },
	{ object4: [200, [305, 299, 400], 100] },
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
	}];
	insertData( dbcl, doc1 );

	//update use addtoset,no matches
	var updateCondition1 = {
		$addtoset: {
			object1: [25, 10, 35, 30, 30, 15, 15],
			object2: [100, 105],
			object3: [1000, 1005],
			"object4.1": [200, 305, 105, 500, 500],
			object5: [600, 605],
			"object6.name": [500, 505]
		}
	};
	updateData( dbcl, updateCondition1 )

	//check result
	var expRecs1 = [{
		object1: [10, 30, 20, 15, 25, 35],
		object2: [100, 105],
		object3: [1000, 1005],
		object4: { 1: [105, 200, 305, 500] },
		object5: [600, 605],
		object6: { name: [500, 505] }
	},
	{
		object1: [10, 15, 25, 30, 35],
		object2: 12,
		object3: [1000, 1005],
		object4: { 1: [105, 200, 305, 500] },
		object5: [600, 605],
		object6: { name: [500, 505] }
	},
	{
		object1: [10, 15, 25, 30, 35],
		object2: [100, 105],
		object3: [1000, 1005],
		object4: [200, [305, 299, 400, 105, 200, 500], 100],
		object5: [600, 605],
		object6: { name: [500, 505] }
	},
	{
		object1: [10, 15, 25, 30, 35],
		object2: [100, 105],
		object3: [1000, 1005],
		object4: { 1: [105, 200, 305, 500] },
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
			600,
			605],
		object6: { name: [500, 505] }
	}];
	checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

	//delete all data
	deleteData( dbcl, null )

	//insert data
	var doc2 = [{ object1: [10, 30, 20] },
	{ object2: 12 },
	{ object4: [200, [305, 299, 400], 100] },
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
	}];
	insertData( dbcl, doc2 );

	//update use addtoset,with matches
	var updateCondition2 = {
		$addtoset: {
			object1: [100, 30, 101],
			object7: [100, 105]
		}
	};
	var findCondition2 = { object1: { $exists: 1 } };
	updateData( dbcl, updateCondition2, findCondition2 )

	//check result
	var expRecs2 = [{ object1: [10, 30, 20, 100, 101], object7: [100, 105] },
	{ object2: 12 },
	{ object4: [200, [305, 299, 400], 100] },
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
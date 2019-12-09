/************************************
*@Description: update any object(exist or not exist) use operator pop
*@author:      zhaoyu
*@createdate:  2016.5.18
**************************************/
function main ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

	//insert arr and common data   
	var doc1 = [{ object1: [10, 30, 20] },
	{ object2: 12 },
	{ object3: [10, 30, 20, 15, 99, 80] },
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
	},
	{
		object8: [-2147483640,
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
	{ object9: [200, [305, 299, 400], 100] },
	{ object10: [200, [305, 299, 400], 100] },
	{ object11: [10, 30, 20] }];
	insertData( dbcl, doc1 );

	//update use pop,no matches
	var updateCondition1 = {
		$pop: {
			"object8.1.2": 5, object6: -5, object7: 0,
			object2: 2,
			object1: 4, object3: 2, object11: 0,
			object5: -3, object8: -11,
			"object4.1": 1, "object9.1": -1, object10: 0
		}
	};
	updateData( dbcl, updateCondition1 );

	//check result
	var expRecs1 = [{ object1: [] },
	{ object2: 12 },
	{ object3: [10, 30, 20, 15] },
	{ object4: [200, [305, 299], 100] },
	{
		object5: ["string",
			{ $oid: "573920accc332f037c000013" },
			false,
			{ $date: "2016-05-16" },
			{ $timestamp: "2016-05-16-13.14.26.124233" },
			{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
			{ $regex: "^z", $options: "i" },
			null]
	},
	{ object8: [] },
	{ object9: [200, [299, 400], 100] },
	{ object10: [200, [305, 299, 400], 100] },
	{ object11: [10, 30, 20] }];
	checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

	//update use addtoset,with matches
	var updateCondition2 = {
		$pop: {
			"object8.1.2": 5, object6: -5, object7: 0,
			object2: 2,
			object1: 4, object3: 2, object11: 0,
			object5: -3, object8: -11,
			"object4.1": 1, "object9.1": -1, object10: 0
		}
	};
	var findCondition2 = { object3: { $exists: 1 } };
	updateData( dbcl, updateCondition2, findCondition2 );

	//check result
	var expRecs2 = [{ object1: [] },
	{ object2: 12 },
	{ object3: [10, 30] },
	{ object4: [200, [305, 299], 100] },
	{
		object5: ["string",
			{ $oid: "573920accc332f037c000013" },
			false,
			{ $date: "2016-05-16" },
			{ $timestamp: "2016-05-16-13.14.26.124233" },
			{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
			{ $regex: "^z", $options: "i" },
			null]
	},
	{ object8: [] },
	{ object9: [200, [299, 400], 100] },
	{ object10: [200, [305, 299, 400], 100] },
	{ object11: [10, 30, 20] }];
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
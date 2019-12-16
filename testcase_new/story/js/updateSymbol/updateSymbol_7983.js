/************************************
*@Description: basic function of operator inc,out of range
*@author:      zhaoyu
*@createdate:  2016.5.16
**************************************/
function main ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

	//insert numberic data   
	var doc1 = [{ a: -2147483648 },
	{ a: 2147483647 },
	{ a: { $numberLong: "-9223372036854775808" } },
	{ a: { $numberLong: "9223372036854775807" } },
	{ a: -1.7E+308 },
	{ a: 1.7E+308 },
	{ a: -4.9E-324 },
	{ a: 4.9E-324 }];
	insertData( dbcl, doc1 );

	//update use $inc,result out of range
	var updateCondition1 = { $inc: { a: -1 } };
	updateData( dbcl, updateCondition1 );
	var expRecs1 = [{ a: -2147483649 },
	{ a: 2147483646 },
	{ a: { $decimal: "-9223372036854775809" } },
	{ a: { $numberLong: "9223372036854775806" } },
	{ a: -1.7E+308 },
	{ a: 1.7E+308 },
	{ a: -1 },
	{ a: -1 }];

	//check result
	var expRecsFindByType1 = [{ a: -2147483649 },
	{ a: { $numberLong: "9223372036854775806" } }];
	var expRecsFindByDecimailType1 = [{ a: { $decimal: "-9223372036854775809" } }];
	checkResult( dbcl, null, null, expRecs1, { _id: 1 } );
	checkResult( dbcl, { a: { $type: 1, $et: 18 } }, null, expRecsFindByType1, { _id: 1 } );
	checkResult( dbcl, { a: { $type: 1, $et: 100 } }, null, expRecsFindByDecimailType1, { _id: 1 } );

	//update use $inc,result out of range
	var updateCondition2 = { $inc: { a: 2 } };
	updateData( dbcl, updateCondition2 );

	//check result
	var expRecs2 = [{ a: -2147483647 },
	{ a: 2147483648 },
	{ a: { $decimal: "-9223372036854775807" } },
	{ a: { $decimal: "9223372036854775808" } },
	{ a: -1.7E+308 },
	{ a: 1.7E+308 },
	{ a: 1 },
	{ a: 1 }];
	var expRecsFindByType2 = [{ a: -2147483647 }, { a: 2147483648 }];
	var expRecsFindByDecimailType2 = [{ a: { $decimal: "-9223372036854775807" } },
	{ a: { $decimal: "9223372036854775808" } }]
	checkResult( dbcl, null, null, expRecs2, { _id: 1 } );
	checkResult( dbcl, { a: { $type: 1, $et: 18 } }, null, expRecsFindByType2, { _id: 1 } );
	checkResult( dbcl, { a: { $type: 1, $et: 100 } }, null, expRecsFindByDecimailType2, { _id: 1 } );

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
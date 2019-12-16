/************************************
*@Description: update data use operator inc,operate object is array or non array, not exist or exist;
*@author:      zhaoyu
*@createdate:  2016.5.16
**************************************/
function main ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

	//insert numberic data,array with 3 layer and common object  
	var doc1 = [{ a: -2147483640, c: -2147483640, e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: { $decimal: "1" }, lastNumber: { $numberLong: "2" } } } },
	{ b: 2147483640, c: 2147483640, d: [1, 2, 3], f: { name: { firstNumber: { $decimal: "1" }, lastNumber: { $numberLong: "2" } } } },
	{ a: { $numberLong: "-9223372036854775800" }, b: { $numberLong: "-9223372036854775800" }, d: [1, 2, 3], e: { name: { firstName: "han", lastName: "meimei" } } },
	{ b: { $numberLong: "9223372036854775800" }, c: { $numberLong: "123" } },
	{ a: -1.7E+308, c: -1.7E+308 },
	{ a: 1.7E+308, b: 1.7E+308 },
	{ b: -4.9E-324, c: -4.9E-324 },
	{ a: 4.9E-324, c: 4.9E-324 }];
	insertData( dbcl, doc1 );

	//update use $inc,the operate object is exist or not exist
	var updateCondition1 = { $inc: { a: 1, b: { $numberLong: "-1" }, c: 1.56789, "d.0": { $decimal: "2" }, "e.name.firstName": 100, "f.name.firstNumber": 1000 } };
	updateData( dbcl, updateCondition1 )

	//check result
	var expRecs1 = [{ a: -2147483639, b: -1, c: -2147483638.43211, d: { 0: { $decimal: "2" } }, e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
	{ a: 1, b: 2147483639, c: 2147483641.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: 100 } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
	{ a: { $numberLong: "-9223372036854775799" }, b: { $numberLong: "-9223372036854775801" }, c: 1.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1, b: { $numberLong: "9223372036854775799" }, c: 124.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: -1.7E+308, b: -1, c: -1.7E+308, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1.7E+308, b: 1.7E+308, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } }];
	checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

	//insert data for updating data with matches symbol
	var doc2 = { g: 1 }
	insertData( dbcl, doc2 );

	//update when matches condition
	var updateCondition2 = { $inc: { g: 1, h: 1 } };
	var findCondition2 = { g: { $et: 1 } };
	updateData( dbcl, updateCondition2, findCondition2 );

	//check result
	var expRecs2 = [{ a: -2147483639, b: -1, c: -2147483638.43211, d: { 0: { $decimal: "2" } }, e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
	{ a: 1, b: 2147483639, c: 2147483641.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: 100 } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
	{ a: { $numberLong: "-9223372036854775799" }, b: { $numberLong: "-9223372036854775801" }, c: 1.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1, b: { $numberLong: "9223372036854775799" }, c: 124.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: -1.7E+308, b: -1, c: -1.7E+308, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1.7E+308, b: 1.7E+308, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
	{ g: 2, h: 1 }];
	checkResult( dbcl, null, null, expRecs2, { _id: 1 } );

	//match nothing and update nothing
	var updateCondition3 = { $inc: { g: 1, h: 1 } };
	var findCondition3 = { g: { $et: 1 } };
	updateData( dbcl, updateCondition3, findCondition3 );

	//check result
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
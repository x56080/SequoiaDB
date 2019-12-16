/************************************
*@Description: seqDB-20101:使用$inc更新符指定Value更新字段值，字段值使用对象类型
*@author:      zhaoyu
*@createdate:  2019.10.29
**************************************/
function main ()
{
	var clName = COMMCLNAME + "_20101";
	commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );
	var cl = commCreateCL( db, COMMCSNAME, clName, { StrictDataMode: true } );
	commCreateIndex( cl, "a_20101", { a: 1 }, false );
	var doc = [{ id: 1, a: 1 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, b: 1 }];
	cl.insert( doc );

	//value值使用int
	var value = 1;
	cl.update( { $inc: { a: { Value: value } } } );
	var expRecs = [{ id: 1, a: 2 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: 1, b: 1 }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//value值使用double
	cl.insert( doc );
	value = 100.12;
	cl.update( { $inc: { a: { Value: value } } } );
	expRecs = [{ id: 1, a: 101.12 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: 100.12, b: 1 }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//value值使用numberLong
	cl.insert( doc );
	value = { $numberLong: "-9223372036854775808" };
	cl.update( { $inc: { a: { Value: value } } } );
	expRecs = [{ id: 1, a: { $numberLong: "-9223372036854775807" } }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: { $numberLong: "-9223372036854775808" }, b: 1 }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//value值使用decimal,SEQUOIADBMAINSTREAM-5130
	cl.insert( doc );
	value = { $decimal: "9223372036854775808" };
	cl.update( { $inc: { a: { Value: value } } } );
	expRecs = [{ id: 1, a: { $decimal: "9223372036854775809" } }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: { $decimal: "9223372036854775808" }, b: 1 }];
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//value为null
	invalidDataUpdateCheckResult( cl, { $inc: { a: { Value: null } } }, -6 );

	commDropCL( db, COMMCSNAME, clName );
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

/************************************
*@Description: seqDB-20102:使用$inc更新符指定Value、Default值更新字段值，字段值使用对象类型
*@author:      zhaoyu
*@createdate:  2019.10.29
**************************************/
function main ()
{
	var clName = COMMCLNAME + "_20102";
	commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );
	var cl = commCreateCL( db, COMMCSNAME, clName, { StrictDataMode: true } );
	commCreateIndex( cl, "a_20102", { a: 1 } );

	//a字段为数值/null/非数值类型/不存在，Value为数值，Default为数值
	var doc = [{ id: 1, a: 1 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4 }];
	cl.insert( doc );
	var defaultValue = 10;
	cl.update( { $inc: { a: { Value: 1, Default: defaultValue } } } );
	var expRecs = [{ id: 1, a: 2 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: 11 }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//a字段为数值/null/非数值类型/不存在，Value为数值，Default为null
	cl.insert( doc );
	defaultValue = null;
	cl.update( { $inc: { a: { Value: 1, Default: defaultValue } } } );
	expRecs = [{ id: 1, a: 2 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4 }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//default为double
	doc = [{ id: 1 }];
	cl.insert( doc );
	defaultValue = 100.12;
	cl.update( { $inc: { a: { Value: 1, Default: defaultValue } } } );
	expRecs = [{ id: 1, a: 101.12 }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//default为numberLong
	doc = [{ id: 1 }];
	cl.insert( doc );
	defaultValue = { $numberLong: "-9223372036854775808" };
	cl.update( { $inc: { a: { Value: 1, Default: defaultValue } } } );
	expRecs = [{ id: 1, a: { $numberLong: "-9223372036854775807" } }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//default为decimal
	doc = [{ id: 1 }];
	cl.insert( doc );
	defaultValue = { $decimal: "9223372036854775808" };
	cl.update( { $inc: { a: { Value: 1, Default: defaultValue } } } );
	expRecs = [{ id: 1, a: { $decimal: "9223372036854775809" } }]
	checkResult( cl, null, null, expRecs, { id: 1 } );
	cl.remove();

	//default为其他类型
	invalidDataUpdateCheckResult( cl, { $inc: { a: { Value: 1, Default: "a" } } }, -6 );

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
/************************************
*@Description: argument actual scale and max scale check for {$decimal:"xxx",$precision:[xx,xx]}
*@author:      zhaoyu
*@createdate:  2016.5.4
**************************************/
function main ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

	//insert data
	var doc = [{ a: { $decimal: "123.12", $precision: [5, 2] } },
	{ a: { $decimal: "456.45", $precision: [6, 3] } },
	{ a: { $decimal: "789.12", $precision: [6, 0] } },
	{ a: { $decimal: "432.55", $precision: [6, 1] } }];
	insertData( dbcl, doc );

	//valid argument check
	var expRecs = [{ a: { $decimal: "123.12", $precision: [5, 2] } },
	{ a: { $decimal: "456.450", $precision: [6, 3] } },
	{ a: { $decimal: "789", $precision: [6, 0] } },
	{ a: { $decimal: "432.6", $precision: [6, 1] } }];
	checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );
}

main();
/****************************************************
@description:	select with [group by] by SQL, basic case
         testlink cases:   seqDB-7423
@input:        1 insert into records
               2 select with [group by]
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName = CHANGEDPREFIX + "_bar";

println( "------Begin to ready cl." );
try
{
	// db.execUpdate("create collection "+csName+"."+clName);
	commDropCL( db, csName, clName, true, true, "drop cl in begin" );
	var opt = { ReplSize: 0 };
	var varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
}
catch( e )
{
	println( "Failed to drop/create cl in the begin." );
	throw e;
}

println( "------Begin to insert into records." );
try 
{
	db.execUpdate( "insert into " + csName + "." + clName + "(price,country) values(1,\"China\")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(price,country) values(4,\"China\")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(price,country) values(5,\"China\")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(price,country) values(6,\"USA\")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(price,country) values(4,\"USA\")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(price,country) values(10,\"English\")" );
}
catch( e )
{
	println( "Failed to insert records." );
	throw e;
}

println( "------Begin to select with [group by]." );
var rc;
try
{
	rc = db.exec( "select country,sum(price) as sum_price from " + csName + "." + clName + " group by country" );
}
catch( e )
{
	println( "Failed to select with [group by]." );
	throw e;
}

println( "------Begin to check results." );
i = 0;
while( rc.next() )
{
	i++;
	if( 10 != rc.current().toObj()["sum_price"] )
		throw "Failed to compare results.";
}
if( i !== 3 )
	throw "Failed to compare results. Expect number: " + expectNum + ",actual number: " + i;

println( "------Begin to drop cl in the end." );
try
{
	db.execUpdate( "drop collection " + csName + "." + clName );
}
catch( e )
{
	println( "Failed to drop cl in the end." );
	throw e;
}
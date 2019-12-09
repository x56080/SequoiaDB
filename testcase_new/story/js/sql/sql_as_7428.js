/****************************************************
@description:	select with [as] by SQL, basic case
         testlink cases:   seqDB-7428
@input:        1 insert into records
               2 select with [as]
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
	db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tom\",10)" );
	db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Miko\",20)" );
}
catch( e )
{
	println( "Failed to insert records." );
	throw e;
}

println( "------Begin to select with [as]." );
var rc;
try
{
	rc = db.exec( "select bar.name as newName from " + csName + "." + clName + " as bar where bar.age=10" );
}
catch( e )
{
	println( "Failed to select with [as]." );
	throw e;
}

println( "------Begin to check results." );
if( "Tom" !== rc.current().toObj()["newName"] )
{
	throw "Failed to check results.";
}

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
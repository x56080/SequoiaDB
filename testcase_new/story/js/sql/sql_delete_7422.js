/****************************************************
@description:	delete by SQL, basic case
         testlink cases:   seqDB-7422
@input:        1 insert into records
               2 delete one record with condition
               3 delete without condition
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
	db.execUpdate( "insert into " + csName + "." + clName + "(name,age) values(\"Tom\"," + 1 + ")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(name,age) values(\"Mike\"," + 2 + ")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(name,age) values(\"Lisa\"," + 3 + ")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(name,age) values(\"Json\"," + 4 + ")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(name,age) values(\"Jhon\"," + 5 + ")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(name,age) values(\"Tina\"," + 6 + ")" );
	db.execUpdate( "insert into " + csName + "." + clName + "(name,age) values(\"Pite\"," + 7 + ")" );
}
catch( e )
{
	println( "Failed to insert records." );
	throw e;
}

println( "------Begin to delete one record with condition." );
try
{
	db.execUpdate( "delete from " + csName + "." + clName + " where age=3" );
}
catch( e )
{
	println( "Failed to delete." );
	throw e;
}

println( "------Begin to check results." );
var rc;
try
{
	rc = db.exec( "select * from " + csName + "." + clName + " where age=3" )
}
catch( e )
{
	println( "Failed to exec select." );
	throw e;
}
//compare results
if( 0 != rc.size() )
{
	throw "Failed to compare results.";
}

println( "------Begin to delete without condition." );
try
{
	db.execUpdate( "delete from " + csName + "." + clName );
}
catch( e )
{
	println( "Failed to delete." );
	throw e;
}

println( "------Begin to check results." );
var rc;
try
{
	rc = db.exec( "select * from " + csName + "." + clName )
}
catch( e )
{
	println( "Failed to exec select." );
	throw e;
}
//compare results
if( 0 != rc.size() )
{
	throw "Failed to compare results.";
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

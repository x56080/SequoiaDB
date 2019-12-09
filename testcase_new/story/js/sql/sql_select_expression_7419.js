/****************************************************
@description:	select by SQL, basic case
         testlink cases:   seqDB-7419
@input:        1 insert into records
               2 select * from local_test_cs.local_test_bar
               3 select age from local_test_cs.local_test_bar
               4 select * from local_test_cs.local_test_barwhere age>10
               5 select _id,age from local_test_cs.local_test_barwhere age<10
               6 select name,age from local_test_cs.local_test_barwhere age=10
               7 select name,age from local_test_cs.local_test_barwhere age>=10
               8 select name,age from local_test_cs.local_test_barwhere age<=10
               9 select * from local_test_cs.local_test_barwhere name="Tom" and age<>5
               10 select * from local_test_cs.local_test_barwhere age>10 and age<15 order by age desc
               11 select * from local_test_cs.local_test_barwhere age<5 or age>15 order by age asc
@output:       return success, and results correct.
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
for( var i = 0; i < 20; i++ )
{
   try
   {
      db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tom\"," + i + ")" );
   }
   catch( e )
   {
      println( "Failed to insert records." );
      throw e;
   }
}

println( "------Begin to select * from " + csName + "." + clName );
var rc;
try
{
   rc = db.exec( "select * from " + csName + "." + clName );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 20 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select age from " + csName + "." + clName );
var rc;
try
{
   rc = db.exec( "select age from " + csName + "." + clName );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 20 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select * from " + csName + "." + clName + "where age>10" );
var rc;
try
{
   rc = db.exec( "select * from " + csName + "." + clName + " where age>10" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 9 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select _id,age from " + csName + "." + clName + "where age<10" );
var rc;
try
{
   rc = db.exec( "select _id,age from " + csName + "." + clName + " where age<10" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 10 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select name,age from " + csName + "." + clName + "where age=10" );
var rc;
try
{
   rc = db.exec( "select name,age from " + csName + "." + clName + " where age=10" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}

if( 1 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select name,age from " + csName + "." + clName + "where age>=10" );
var rc;
try
{
   rc = db.exec( "select name,age from " + csName + "." + clName + " where age>=10" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 10 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select name,age from " + csName + "." + clName + "where age<=10" );
var rc;
try
{
   rc = db.exec( "select name,age from " + csName + "." + clName + " where age<=10" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 11 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select * from " + csName + "." + clName + "where name=\"Tom\" and age<>5" );
var rc;
try
{
   rc = db.exec( "select * from " + csName + "." + clName + " where name=\"Tom\" and age<>5" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 19 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select * from " + csName + "." + clName + "where age>10 and age<15 order by age desc" );
var rc;
try
{
   rc = db.exec( "select * from " + csName + "." + clName + " where age>10 and age<15 order by age desc" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 4 != rc.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to select * from " + csName + "." + clName + "where age<5 or age>15 order by age asc" );
var rc;
try
{
   rc = db.exec( "select * from " + csName + "." + clName + " where age<5 or age>15 order by age asc" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}

if( 9 != rc.size() )
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
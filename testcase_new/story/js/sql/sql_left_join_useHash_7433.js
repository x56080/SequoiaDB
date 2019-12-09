/****************************************************
@description:	left join with /*+use_hash()*/
/*        testlink cases:   seqDB-7433
@input:        1 insert into records
               2 select with [left join, use_hash, order by A.id]
               3 select with [left join, use_hash, order by A.id desc]
/*
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName1 = CHANGEDPREFIX + "_student";
clName2 = CHANGEDPREFIX + "_grade";

println( "------Begin to ready cl." );
try
{
   // db.execUpdate("create collection "+csName+"."+clName);
   commDropCL( db, csName, clName1, true, true, "drop cl in begin" );
   commDropCL( db, csName, clName2, true, true, "drop cl in begin" );
   var opt = { ReplSize: 0 };
   var cl_pro = commCreateCLByOption( db, csName, clName1, opt, true, false, "create cl in begin" );
   var cl_name = commCreateCLByOption( db, csName, clName2, opt, true, false, "create cl in begin" );
}
catch( e )
{
   println( "Failed to drop/create cl in the begin." );
   throw e;
}

println( "------Begin to insert into records." );
try
{
   for( var i = 1; i <= 100; i++ )
   {
      db.execUpdate( "insert into " + csName + "." + clName1 + "(name,id) values(\"A" + i + "\"," + i + ")" );
   }
   for( var i = 1; i <= 50; i++ )
   {
      db.execUpdate( "insert into " + csName + "." + clName2 + "(user,id) values(\"B" + i + "\"," + i + ")" );
   }
}
catch( e )
{
   println( "Failed to insert records." )
   throw e;
}

println( "------Begin to exec select with [left outer join, use_hash, order by A.id]." );
try
{
   var res = db.exec( "select A.name , B.user , A.id from " + csName + "." + clName1 + " as A left outer join " + csName + "." + clName2 + " as B on A.id=B.id order by A.id asc /*+use_hash()*/" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}

println( "------Begin to check results." )
var number = 0;
while( res.next() )
{
   number++;
}
if( 100 != number )
{
   println( "Failed to check results. Expect number: 100, actual number: " + number );
   throw "Error.";
}

println( "------Begin to exec select with [left outer join, use_hash, order by A.id desc]." );
try
{
   var res = db.exec( "select A.name , B.user , B.id from " + csName + "." + clName1 + " as A left outer join " + csName + "." + clName2 + " as B on A.id=B.id order by A.id desc /*+use_hash()*/" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}

println( "------Begin to check results." )
var value = res.current().toObj();
if( value["name"] !== "A100" )
{
   throw "Failed to compare the fields.";
}
var number = 1;
while( res.next() )
{
   number++;
}
if( 100 != number )
{
   println( "Failed to compare the count. Expect number: 100, actual number: " + number );
   throw "Error.";
}

println( "------Begin to drop cl in the end." );
try
{
   db.execUpdate( "drop collection " + csName + "." + clName1 );
   db.execUpdate( "drop collection " + csName + "." + clName2 );
}
catch( e )
{
   println( "Failed to drop cl in the end." );
   throw e;
}
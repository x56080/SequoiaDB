/****************************************************
@description:	create/drop CS by SQL, basic case
         testlink cases:   seqDB-7407/7409/7416
@input:        1 create cs, success
               2 exec [list collectionspases], success
               3 create the same cs again, errorno: -33
               4 drop cs, success
               5 create cs again, success
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
var csName = CHANGEDPREFIX + "_foo";

println( "------Begin to clean in the begin." );
try
{
   db.execUpdate( "drop collectionspace " + csName );
}
catch( e )
{
   if( e != -34 )
   {
      println( "Failed to clean env in the begin." );
      throw e;
   }
}

println( "------Begin to create cs." );
try
{
   db.execUpdate( "create collectionspace " + csName );
}
catch( e )
{
   println( "Failed to create cs." );
   throw e;
}

println( "------Begin to check results." );
try
{
   var rc = db.getCS( csName );
}
catch( e )
{
   println( "Failed to create cs." );
   throw e;
}

println( "------Begin to exec [list collectionspaces]." );
try
{
   var rc = db.exec( "list collectionspaces" );
}
catch( e )
{
   println( "Failed to exec [list collectionspaces]." );
   throw e;
}
//compare results
if( 0 == rc.size() )
   throw "Failed to compare results.";

println( "------Begin to create the same cs repeat." );
try
{
   db.execUpdate( "create collectionspace " + csName );
   throw "Create duplicate cs success. Expect errorno: -33";
}
catch( e )
{
   if( e !== -33 )
   {
      throw e;
   }
}

println( "------Begin to drop the cs." );
try
{
   db.execUpdate( "drop collectionspace " + csName );
}
catch( e )
{
   println( "Failed to drop cs." );
   throw e;
}

println( "------Begin to create the cs again." );
try
{
   db.execUpdate( "create collectionspace " + csName );
}
catch( e )
{
   println( "Failed to create cs." );
   throw e;
}

try
{
   var rc = db.getCS( csName );
}
catch( e )
{
   println( "Failed to create cs again." );
   throw e;
}

println( "------Begin to drop cs in the end." );
try
{
   db.execUpdate( "drop collectionspace " + csName );
}
catch( e )
{
   println( "Failed to clear env." );
   throw e;
}

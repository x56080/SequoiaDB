/****************************************************
@description:	create/drop CL by SQL, basic case
         testlink cases:   seqDB-7410/7412/7417
@input:        1 The cs does not exist, create cl in the cs. errorno: -34
               2 create cl, success
               3 exec [list collections], success
               4 create the same cl again, errorno: -33
               5 drop cl, success
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
var tmpCS = CHANGEDPREFIX + "_foo"
var csName = COMMCSNAME;
var clName = CHANGEDPREFIX + "_bar";

println( "------Begin to clean env in the begin." );
try  //clean cl
{
   db.execUpdate( "drop collection " + csName + "." + clName );
}
catch( e )
{
   if( e != -23 )
   {
      println( "Failed to clean env in the begin." );
      throw e;
   }
}

println( "------Begin to create cl when the cs does not exist." );
try
{
   db.execUpdate( "create collection " + tmpCS + "." + clName );
}
catch( e )
{
   if( e !== -34 )
   {
      println( "Failed to create CL in the CS when the CS does not exist. Expected errorno: -34" );
      throw e;
   }
}

println( "------Begin to create nomal cl." );
try
{
   db.execUpdate( "create collection " + csName + "." + clName );
}
catch( e )
{
   println( "Failed to create CL." );
   throw e;
}

println( "------Begin to check results." );
try
{
   db.getCS( csName ).getCL( clName );
}
catch( e )
{
   println( "Failed to get CL " + csName + "." + clName );
   throw e;
}

println( "------Begin to exec [list collections]." );
try
{
   var rc = db.exec( "list collections" );
}
catch( e )
{
   println( "Failed to exec [list collections]." );
   throw e;
}
//compare results
if( 0 == rc.size() )
   throw "Failed to compare results.";

println( "------Begin to create the same cl repeat." );
try
{
   db.execUpdate( "create collection " + csName + "." + clName );
}
catch( e )
{
   if( e !== -22 )
   {
      println( "create the same CL again,success. Expected errorno: -22" );
      throw e;
   }
}

println( "------Begin to drop the cl." );
try 
{
   db.execUpdate( "drop collection " + csName + "." + clName );
}
catch( e )
{
   println( "Failed to drop cl." );
   throw e;
}

println( "------Begin to check results after drop cl." );
try
{
   db.getCS( csName ).getCL( clName );
}
catch( e )
{
   if( e !== -23 )
   {
      println( "Failed to compare result[drop cl]." );
      throw e;
   }
}
/****************************************************
@description:	create/drop index, basic case
         testlink cases:   seqDB-7413/7415
@input:        1 insert into records
               2 drop index when it does not exist, errorno: -47
               3 create/drop nomal index, success
               4 create the same index repeat, errorno: -247
               5 create/drop index, the indexKey contains two fields, success
               6 create/drop unique index, success
               7 drop the same index repeat, errorno: -47       
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName = CHANGEDPREFIX + "_bar";
indexName = CHANGEDPREFIX + "_ix";

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

println( "------Begin to drop index when it does not exist." );
try
{
   db.execUpdate( "drop index " + indexName + " on " + csName + "." + clName );
}
catch( e )
{
   if( e !== -47 )
   {
      throw e;
   }
}

println( "------Begin to create nomal index." );
try
{
   db.execUpdate( "create index " + indexName + " on " + csName + "." + clName + " (age desc)" );
}
catch( e )
{
   println( "Failed to create index by age desc." );
   throw e;
}

println( "------Begin to create the same index repeat." );
try
{
   db.execUpdate( "create index " + indexName + " on " + csName + "." + clName + " (age desc)" );
   throw "Repeat to create the same index, success. Expected errorno: -247";
}
catch( e )
{
   if( e !== -247 )
   {
      throw e;
   }
}

println( "------Begin to drop the index." );
try
{
   db.execUpdate( "drop index " + indexName + " on " + csName + "." + clName );
}
catch( e )
{
   println( "Failed to drop index." );
   throw e;
}

println( "------Begin to create index, the indexKey contains two fields." );
try
{
   db.execUpdate( "create index " + indexName + " on " + csName + "." + clName + " (age,name)" );
}
catch( e )
{
   println( "Failed to create index used two fields." );
   throw e;
}

println( "------Begin to drop the index." );
try
{
   db.execUpdate( "drop index " + indexName + " on " + csName + "." + clName );
}
catch( e )
{
   println( "Failed to drop index." );
   throw e;
}

println( "------Begin to create unique index." );
try
{
   db.execUpdate( "create unique index " + indexName + " on " + csName + "." + clName + " (age)" );
}
catch( e )
{
   println( "Failed to create unique index." );
   throw e;
}

println( "------Begin to drop the index." );
try
{
   db.execUpdate( "drop index " + indexName + " on " + csName + "." + clName );
}
catch( e )
{
   println( "Failed to drop the unique index." );
   throw e;
}

println( "------Begin to drop the same index repeat." );
try
{
   db.execUpdate( "drop index " + indexName + " on " + csName + "." + clName );
   throw "Repeat to drop the same index, success. Expected errorno: -47";
}
catch( e )
{
   if( e !== -47 )
   {
      throw e;
   }
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
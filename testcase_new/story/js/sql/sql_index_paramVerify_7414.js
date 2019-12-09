/****************************************************
@description:	create/drop index by SQL, verify parameters
         testlink cases:   seqDB-7414/7415
@input:        1 insert into records
               2 create/drop index, the name cotains invalid characters, errorno: -6
               3 create/drop index, the name is " ", errorno: -6
               4 create index, the indexKey without fields, errorno: -195
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

println( "------Begin to create index, the name cotains invalid characters." );
var aa = Array( "$", ".", "a." );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var specIndexName = aa[i] + CHANGEDPREFIX;
      db.execUpdate( "create index " + specIndexName + " on " + csName + "." + clName + " (name)" );
      //expect errorno: -6 ,if create success then throw
      throw "The name contains invalid characters,create index success. Expect errorno: -6 ";
   } catch( e )
   {
      if( e !== -6 )
         throw e;
   }
}

println( "------Begin to drop index, the name cotains invalid characters." );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var specIndexName = aa[i] + CHANGEDPREFIX;
      db.execUpdate( "drop index " + specIndexName + " on " + csName + "." + clName );
      //expect errorno: -6 ,if create success then throw
      throw "The name contains invalid characters,create index success. Expect errorno: -6 ";
   } catch( e )
   {
      if( e !== -47 )
         throw e;
   }
}

println( '------Begin to create index, the name is " ".' );
try
{
   db.execUpdate( "create index " + "  on " + csName + "." + clName + " (name)" );
}
catch( e )
{
   if( e !== -195 )
   {
      throw e;
   }
}

println( '------Begin to drop index, the name is " ".' );
try
{
   db.execUpdate( "drop index " + "  on " + csName + "." + clName );
}
catch( e )
{
   if( e !== -195 )
   {
      throw e;
   }
}

println( "------Begin to create index, the indexKey without fields." );
try
{
   db.execUpdate( "create index " + indexName + " on " + csName + "." + clName );
}
catch( e )
{
   if( e !== -195 )
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
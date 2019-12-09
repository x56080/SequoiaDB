/****************************************************
@description:	insert into by SQL, basic case
         testlink cases:   seqDB-7418
@input:        1 insert into records, success
               2 insert into records ,the key contains invalid character, errorno: -195
               3 insert into records ,the value contains valid character, success    
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
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Mike\",20)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (_id,a) values (\"123\",10)" );
}
catch( e )
{
   println( "Failed to insert basic record." );
   throw e;
}

println( "------Begin to check result." );
try
{
   var rc1 = db.exec( "select name,age from " + csName + "." + clName + " where name=\"Mike\" and age=20" );
   var rc2 = db.exec( "select _id,a from " + csName + "." + clName + " where _id=\"123\" and a=10" );
}
catch( e )
{
   println( "Failed to exec select." );
   throw e;
}
if( 1 != rc1.size() || 1 != rc2.size() )
{
   throw "Failed to check results.";
}

println( "------Begin to insert into records ,the key contains invalid character." )
var aa = Array( "=", ">", "<", "*", ";", ",", "\"" );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var age = aa[i] + "age";
      db.execUpdate( "insert into " + csName + "." + clName + "(" + age + ")" + " values(25)" );
   }
   catch( e )
   {
      if( e !== -195 )
      {
         throw e;
      }
   }
}

println( "------Begin to insert into records ,the value contains valid character." )
//,"\'","\""
var aa = Array( ";", ":", "{", "}", "[", "]", ",", "<", ">", "?", "/", "|", "\\", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "*" );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var value = aa[i] + "chen";
      db.execUpdate( "insert into " + csName + "." + clName + "(name) values" + "('" + value + "')" );
   }
   catch( e )
   {
      println( "Failed to insert into, the value contains special character. Expecte: success" );
      throw e;
   }
}

println( "------Begin to check results" );
var cnt = 0;
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var value = aa[i] + "chen";
      var rc3 = db.exec( "select name from " + csName + "." + clName + " where name='" + value + "'" );
      cnt = cnt + rc3.size();
   }
   catch( e )
   {
      println( "Failed to exec select, the value contains special character. Expecte: success" );
      throw e;
   }
}
if( aa.length !== cnt )
{
   throw "Failed to check result. Actual result: " + cnt + ", expect result: " + aa.length;
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
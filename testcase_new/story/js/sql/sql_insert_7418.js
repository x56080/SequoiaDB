/****************************************************
@description:	insert into by SQL, basic case
         testlink cases:   seqDB-7418
@input:        1 insert into records, success
               2 insert into records ,the key contains invalid character, errorno: -195
               3 insert into records ,the value contains valid character, success    
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_7418", testConf.clOpt = { ReplSize: 0 };

main( test );

function test ()
{
   db.execUpdate( "insert into " + testConf.csName + "." + testConf.clName + " (name,age) values (\"Mike\",20)" );
   db.execUpdate( "insert into " + testConf.csName + "." + testConf.clName + " (_id,a) values (\"123\",10)" );

   var rc1 = db.exec( "select name,age from " + testConf.csName + "." + testConf.clName + " where name=\"Mike\" and age=20" );
   var rc2 = db.exec( "select _id,a from " + testConf.csName + "." + testConf.clName + " where _id=\"123\" and a=10" );
   if( 1 != rc1.size() || 1 != rc2.size() )
   {
      throw new Error( "Failed to check results." );
   }

   var aa = Array( "=", ">", "<", "*", ";", ",", "\"" );
   for( var i = 0; i < aa.length; ++i )
   {
      try
      {
         var age = aa[i] + "age";
         db.execUpdate( "insert into " + testConf.csName + "." + testConf.clName + "(" + age + ")" + " values(25)" );
      }
      catch( e )
      {
         if( e.message != -195 )
         {
            throw new Error( e );
         }
      }
   }

   var aa = Array( ";", ":", "{", "}", "[", "]", ",", "<", ">", "?", "/", "|", "\\", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "*" );
   for( var i = 0; i < aa.length; ++i )
   {
      var value = aa[i] + "chen";
      db.execUpdate( "insert into " + testConf.csName + "." + testConf.clName + "(name) values" + "('" + value + "')" );
   }

   var cnt = 0;
   for( var i = 0; i < aa.length; ++i )
   {
      var value = aa[i] + "chen";
      var rc3 = db.exec( "select name from " + testConf.csName + "." + testConf.clName + " where name='" + value + "'" );
      cnt = cnt + rc3.size();
   }
   if( aa.length !== cnt )
   {
      throw new Error( "Failed to check result. Actual result: " + cnt + ", expect result: " + aa.length );
   }

}
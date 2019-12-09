/******************************************************************************
 * @Description : the connection takes effect immediately after the transaction configuration is updated
                  seqDB-18337:事务中更改事务配置后连接立即生效可用
 * @author      : luweikang
 * @date        ：2019.05.04
 ******************************************************************************/

var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );

try
{
   main();
}
finally
{
   db1.transCommit();
   db2.transCommit();
   db1.close();
   db2.close();
   db.deleteConf( { "transisolation": 1 }, { Global: false } );
}

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " run mode is standalone" );
      return;
   }

   var snaphostCur = db.snapshot( 13, { "svcname": COORDSVCNAME.toString() } );
   //println(snaphostCur);
   var obj = snaphostCur.next().toObj();
   snaphostCur.close()
   if( obj.transisolation != 0 )
   {
      db.updateConf( { "transisolation": 0 }, { Global: false } );
   }

   var csName = COMMCSNAME;
   var clName = "cl_18337A";
   var r1 = { "_id": 1, "a": 1 };
   var r2 = { "_id": 2, "a": 2 }

   var cl1 = commCreateCL( db1, csName, clName );
   var cl2 = db2.getCS( csName ).getCL( clName );

   db1.transBegin();
   db2.transBegin();

   cl1.insert( r1 );
   var cursor = cl2.find( { "a": 1 } );
   var record = cursor.next();
   cursor.close();

   if( JSON.stringify( r1 ) != JSON.stringify( record.toObj() ) )
   {
      throw buildException( "", "", "check ru read record", JSON.stringify( r1 ), JSON.stringify( record.toObj() ) );
   }

   //非事务中更改事务配置
   db.updateConf( { "transisolation": 1 }, { Global: false } );

   //更新配置对执行中的事务不生效,需提交本次事务重新开启事务生效
   var cursor = cl2.find( { "a": 1 } );
   var record = cursor.next();
   cursor.close();

   if( JSON.stringify( r1 ) != JSON.stringify( record.toObj() ) )
   {
      throw buildException( "", "", "check ru read record", JSON.stringify( r1 ), JSON.stringify( record.toObj() ) );
   }

   db1.transCommit();
   db2.transCommit();

   db1.transBegin();
   db2.transBegin();

   cl1.insert( r2 );
   var cursor = cl2.find( { "a": 2 } );
   if( cursor.next() )
   {
      throw buildException( "", "", "check rc read record", "no record", cursor.current().toString() );
   }
   cursor.close();

   db1.transCommit();

   var cursor = cl2.find( { "a": 2 } );
   var record = cursor.next();
   cursor.close();
   db2.transCommit();
   if( JSON.stringify( r2 ) != JSON.stringify( record.toObj() ) )
   {
      throw buildException( "", "", "check rc read record", JSON.stringify( r2 ), JSON.stringify( record.toObj() ) );
   }

   db.getCS( csName ).dropCL( clName );
}
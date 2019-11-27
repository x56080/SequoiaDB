/******************************************************************************
@Description :   seqDB-17066:事务连接和非事务连接读取数据
@Modify list :   2019-1-9    xiaoni Zhao  Init
******************************************************************************/
function main()
{
   var clName = COMMCLNAME + "_17066";
   
   commDropCL( db, COMMCSNAME, clName, true, true );
   
   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dbcl1 = commCreateCL( db1, COMMCSNAME, clName );
   var dbcl2 = db2.getCS( COMMCSNAME ).getCL( clName );
   
   db1.transBegin();
   for( i = 0; i < 10000; ++i )
   {
      dbcl1.insert( {"transTest":i} );
   }
   
   var count1 = dbcl1.find( {"transTest": 0} ).count();
   if( count1 != 1 )
   {
      throw new Error( "expect record num: 1, actual record num: " + count1 );
   }
   
   var count2 = dbcl2.find( {"transTest" : 9999} ).count();
   if( count2 != 1 )
   {
      throw new Error( "expect record num: 1, actual record num: " + count2 );
   }
   
   var count3 = dbcl1.find( {"transTest" : 9999} ).count();
   if( count3 != 1 )
   {
      throw new Error( "expect record num: 1, actual record num: " + count3 );
   }
   
   db1.transCommit();
   
   commDropCL( db, COMMCSNAME, clName, true, true );
   db1.close();
   db2.close();
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

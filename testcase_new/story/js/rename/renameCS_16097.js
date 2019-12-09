/************************************
*@Description: 多次修改cs名
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16097
**************************************/

main();

function main ()
{
   //@ clean before
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      println( "--least two groups" );
      return;
   }

   var oldcsName = CHANGEDPREFIX + "_16097_oldcs";
   var newcsName = CHANGEDPREFIX + "_16097_newcs";
   var clName = CHANGEDPREFIX + "_16097_maincl";
   var subclName1 = CHANGEDPREFIX + "_16097_subcl1";
   var subclName2 = CHANGEDPREFIX + "_16097_subcl2";

   println( "---create main sub cl---" );
   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCLByOption( db, oldcsName, clName, { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true }, false, false, "create CL in the begin" );
   var subcl1 = commCreateCLByOption( db, oldcsName, subclName1, { ShardingKey: { no: 1 }, ShardingType: "range" }, false, false, "create CL in the begin" );
   var subcl2 = commCreateCLByOption( db, oldcsName, subclName2, { ShardingKey: { no: 1 }, ShardingType: "hash" }, false, false, "create CL in the begin" );
   cl.attachCL( oldcsName + "." + subclName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   cl.attachCL( oldcsName + "." + subclName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   println( "---insert data---" );
   insertData( cl, 2000 );

   println( "---test rename cs---" );
   for( i = 0; i < 10; i++ )
   {
      db.renameCS( oldcsName, newcsName );
      cl = db.getCS( newcsName ).getCL( clName );
      cl.update( { $set: { c: "test1" } } );
      db.renameCS( newcsName, oldcsName );
      cl = db.getCS( oldcsName ).getCL( clName );
      cl.update( { $set: { c: "test2" } } );
   }

   db.renameCS( oldcsName, newcsName );
   checkRenameCSResult( oldcsName, newcsName, 2 );

   cl = db.getCS( newcsName ).getCL( clName );
   println( "---check record---" );
   checkRecord( cl, 2000 );

   commDropCS( db, newcsName, true, false, "clean cs---" );
}

function checkRecord ( dbcl, recordNum )
{
   var actNum = dbcl.count( { c: "test2" } );
   if( actNum != recordNum )
   {
      throw buildException( "checkRecord()", null, "check the new cl record nums",
         recordNum, actNum );
   }
}


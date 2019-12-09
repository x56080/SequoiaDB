/***************************************************************************
*@Description:    seqDB-10440:数据落在不同组上的不同子表中，批量插入数据
*@author:   zengxianquan  2016.11.23
*@modify:   huangxiaoni   2019.4.15
****************************************************************************/

main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      println( "---Least two groups" );
      return;
   }
   db.setSessionAttr( { PreferedInstance: "M" } );

   mclName = "mcl_10440";
   sclName1 = "scl_1_10440";
   sclName2 = "scl_2_10440";

   //clean environment before test
   commDropCL( db, COMMCSNAME, mclName, true, true, "clean main cl" );
   commDropCL( db, COMMCSNAME, sclName1, true, true, "clean sub cl1" );
   commDropCL( db, COMMCSNAME, sclName2, true, true, "clean sub cl2" );

   //get all groups
   var groups = commGetGroups( db, false, "", false, true, true );
   var groupName1 = groups[1][0].GroupName;
   var groupName2 = groups[2][0].GroupName;

   //create maincl 
   println( "\n---Begin to create cl." );
   var mainCL = commCreateCLByOption( db, COMMCSNAME, mclName, { ShardingKey: { "a": 1 }, IsMainCL: true }, true, true );
   commCreateCLByOption( db, COMMCSNAME, sclName1, { Group: groupName1 }, true, true );
   commCreateCLByOption( db, COMMCSNAME, sclName2, { Group: groupName2 }, true, true );
   //attach subcl
   println( "\n---Begin to attach cl." );
   mainCL.attachCL( COMMCSNAME + "." + sclName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.attachCL( COMMCSNAME + "." + sclName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   // insert
   println( "\n---Begin to insert." );
   var recordsNum = 200;
   var doc = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      doc.push( { a: i } );
   }
   mainCL.insert( doc );

   // check total count
   println( "\n---Begin to check results in main cl." );
   var totalCnt = mainCL.count();
   if( Number( totalCnt ) !== recordsNum ) 
   {
      throw buildException( "main", null, "check results", recordsNum, totalCnt );
   }

   // check rg1 count
   println( "\n---Begin to check results in sub cl1." );
   var rg1 = db.getRG( groupName1 ).getMaster().connect();
   var expRG1Cnt = 100;
   var actRG1Cnt = rg1.getCS( COMMCSNAME ).getCL( sclName1 ).count( { a: { $lt: 100 } } );
   if( Number( actRG1Cnt ) !== expRG1Cnt ) 
   {
      throw buildException( "main()", null, "check results", expRG1Cnt, actRG1Cnt );
   }

   // check rg2 count
   println( "\n---Begin to check results in sub cl2." );
   var rg2 = db.getRG( groupName2 ).getMaster().connect();
   var expRG2Cnt = 100;
   var actRG2Cnt = rg2.getCS( COMMCSNAME ).getCL( sclName2 ).count( { a: { $gte: 100 } } );
   if( Number( actRG2Cnt ) !== expRG2Cnt ) 
   {
      throw buildException( "main", null, "check results", expRG2Cnt, actRG2Cnt );
   }

   // clear environment
   commDropCL( db, COMMCSNAME, mclName, true, false, "clean main cl" );
}
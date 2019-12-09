/******************************************************************************
*@Description : test insert special decimal value to range/hash table
*               seqDB-13998:水平分区表插入特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }
   var groups = getGroupName( db );
   if( groups.length < 2 )
   {
      println( "At least two groups" );
      return;
   }

   // test range split cl
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
   var option = { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0 };
   var cl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, option, true, true );

   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];
   insertData( cl, docs );

   var startCond = { a: 10 };
   var splitGrpInfo = ClSplitOneTimes( COMMCSNAME, COMMCLNAME, startCond );
   var expRecs = [[{ a: { $decimal: "MIN" } }, { a: { $decimal: "NaN" } }],
   [{ a: { $decimal: "MAX" } }]];
   checkRangeClSplitResult( db, COMMCLNAME, splitGrpInfo, {}, {}, expRecs, { _id: 1 } );

   // test hash split cl
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL after test range split" );
   option = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 };
   cl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, option, true, true );


   insertData( cl, docs );

   splitGrpInfo = ClSplitOneTimes( COMMCSNAME, COMMCLNAME, 0.5 );
   var expRecsNum = 3;
   checkHashClSplitResult( db, COMMCLNAME, splitGrpInfo, {}, {}, expRecsNum, { _id: 1 } );
}
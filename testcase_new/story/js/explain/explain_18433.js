/******************************************************************************
*@Description : seqDB-18433:查询时多个索引符合候选计划要求的索引选择
*@author      : Li Yuanyue
*@Date        : 2020.5.12
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_18433";

main( test );

function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   var clName = CHANGEDPREFIX + "_18433";
   var idxName1 = "index_a_18433";
   var idxName2 = "index_ab_18433";
   var idxName3 = "index_abc_18433";

   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: dataGroupNames[0] }, false );

   cl.createIndex( idxName1, { a: 1 } );
   cl.createIndex( idxName2, { a: 1, b: 1 } );
   cl.createIndex( idxName3, { a: 1, b: 1, c: 1 } );

   var docs = [];
   for( var i = 0; i < 11000; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   cl.insert( docs );

   // 不计算io代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1, c: 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var docs = [];
   for( var i = 0; i < 11000; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   cl.insert( docs );

   // 计算io代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1, c: 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   commDropCL( db, COMMCSNAME, clName, false );
}
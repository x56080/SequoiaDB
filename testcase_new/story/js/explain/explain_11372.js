/******************************************************************************
*@Description : seqDB-11372:匹配组合索引的索引选择
*@author      : Li Yuanyue
*@Date        : 2020.4.25
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11372";

main( test );

function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   var clName = CHANGEDPREFIX + "_11372";
   var idxName1 = "index_abc_11372";
   var idxName2 = "index_ab_11372";
   var idxName3 = "index_a_11372";
   var tbIdx = "";

   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: dataGroupNames[0] }, false );

   cl.createIndex( idxName1, { a: 1, b: -1, c: 1 } );
   cl.createIndex( idxName2, { a: -1, b: 1 } );
   cl.createIndex( idxName3, { a: 1 } );

   // 生成随机数
   var rd = new commDataGenerator();
   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze();

   // 不计算io代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1, "b": 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze();

   // 计算io代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1, "b": 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   commDropCL( db, COMMCSNAME, clName, false );
}
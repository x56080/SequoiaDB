/******************************************************************************
*@Description : seqDB-11376:查询并按多个字段排序的索引选择
*@author      : Li Yuanyue
*@Date        : 2020.5.7
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11376";

main( test );

function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   var clName = CHANGEDPREFIX + "_11376";
   var idxName1 = "index_abc_11376";
   var idxName2 = "index_ac_11376";
   var idxName3 = "index_bc_11376";

   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: dataGroupNames[0] }, false );

   cl.createIndex( idxName1, { a: 1, b: -1, c: 1 } );
   cl.createIndex( idxName2, { a: 1, c: -1 } );
   cl.createIndex( idxName3, { b: 1, c: -1 } );

   // 生成随机数
   var rd = new commDataGenerator();
   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze();

   // 不计算 IO 代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1, "b": 1, "c": 1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   var sortCond = { "a": 1, "b": 1, "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "a": 1, "b": 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   var sortCond = { "a": 1, "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "b": 1, "c": 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   var sortCond = { "b": 1, "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze();

   // 计算 IO 代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1, "b": 1, "c": 1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   var sortCond = { "a": 1, "b": 1, "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "a": 1, "b": 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   var sortCond = { "a": 1, "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "b": 1, "c": 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   var sortCond = { "b": 1, "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   commDropCL( db, COMMCSNAME, clName, false );
}
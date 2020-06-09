/******************************************************************************
*@Description : seqDB-11380:hint一个存在的索引进行查询
                seqDB-11381:hint多个索引进行查询
                seqDB-11382:hint非索引进行查询 
*@author      : Li Yuanyue
*@Date        : 2020.5.11
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11380_11382";

main( test );

function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   var clName = CHANGEDPREFIX + "_11380_11382";
   var idxName1 = "index_abc_11380_11382";
   var idxName2 = "index_ac_11380_11382";
   var idxName3 = "index_a_11380_11382";
   var tbIdx = "";
   var notIdx = "cccccc_11380_11382";

   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: dataGroupNames[0] }, false );

   cl.createIndex( idxName1, { a: 1, b: -1, c: 1 } );
   cl.createIndex( idxName2, { a: 1, c: -1 } );
   cl.createIndex( idxName3, { a: 1 } );

   // 生成随机数
   var rd = new commDataGenerator();
   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze();

   // 不计算io代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   // hint一个存在索引
   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   var sortCond = {};
   var hintCond = { "": null };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond, hintCond );

   // hint多个存在索引
   var cond = { "a": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   var sortCond = {};
   var hintCond = { "1": idxName1, "2": idxName3 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond, hintCond );

   // hint非索引
   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   var sortCond = {};
   var hintCond = { "": notIdx };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond, hintCond );

   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze();

   // 计算io代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   // hint一个存在索引
   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   var sortCond = {};
   var hintCond = { "": idxName2 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond, hintCond );

   // hint多个存在索引
   var cond = { "a": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   var sortCond = {};
   var hintCond = { "1": idxName1, "2": null };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond, hintCond );

   // hint非索引
   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   var sortCond = {};
   var hintCond = { "": notIdx };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond, hintCond );

   commDropCL( db, COMMCSNAME, clName, false );
}
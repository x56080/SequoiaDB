/******************************************************************************
 * @Description   : seqDB-34158:RefObj 引用主表对象
 * @Description   : seqDB-34162:RefFrom 引用主表
 * @Description   : seqDB-34163:RefObj / RefFrom 引用带各种非分区选项的集合
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   // ref main cl (RefFrom)
   var mainCLName = 'main_34158';
   var subCLName = 'sub_34158';
   var mcl = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 } } );
   commCreateCL( db, COMMCSNAME, subCLName, {} );
   mcl.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mcl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   var mainCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + mainCLName } ).next().toObj();

   clName = 'cl_34162';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + mainCLName, RefMode: 2 } );
   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   } );
   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj, true );

   for (key in clObj)
   {
      assert.equal( mainCLObj[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // ref main cl (RefObj)
   clName = 'cl_34158';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefObj: mainCLObj, RefMode: 2 } );
   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   } );
   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj, true );

   for (key in clObj)
   {
      assert.equal( mainCLObj[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );
   commDropCL( db, COMMCSNAME, mainCLName );
}

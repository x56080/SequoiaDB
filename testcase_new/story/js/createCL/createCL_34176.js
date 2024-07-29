/******************************************************************************
 * @Description   : seqDB-34176:使用 RefObj / RefFrom 引用普通表、主表同时指定 RefMode
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ()
{
   var normalCLName = 'normal_34176';
   var cl = commCreateCL( db, COMMCSNAME, normalCLName, {
      AutoIncrement: { Field: "id" },
      Compressed: false,
      ConsistencyStrategy: 2,
      AutoIndexId: false,
      StrictDataMode: false } );
   var normalCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + normalCLName } ).next().toObj();

   clName = 'cl_34176_1';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + normalCLName, RefMode: 1 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   normalCLObj4Check = JSON.parse(JSON.stringify(normalCLObj));
   pickFieldAbleToRef( normalCLObj4Check );
   for (key in clObj)
   {
      assert.equal( normalCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );



   var mainCLName = 'main_34176';
   var subCLName = 'sub_34176';
   var mcl = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 } } );
   commCreateCL( db, COMMCSNAME, subCLName, {} );
   mcl.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mcl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   var mainCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + mainCLName } ).next().toObj();

   clName = 'cl_34176_2';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + mainCLName, RefMode: 1 } );
   var clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   assert.equal( clObj.CataInfo, [] );
   delete clObj.CataInfo;
   for (key in clObj)
   {
      assert.equal( mainCLObj[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   commDropCL( db, COMMCSNAME, normalCLName );
   commDropCL( db, COMMCSNAME, mainCLName );
}

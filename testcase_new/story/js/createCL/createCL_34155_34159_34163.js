/******************************************************************************
 * @Description   : seqDB-34155:RefObj 引用普通表对象
 * @Description   : seqDB-34159:RefFrom 引用普通表
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
   // ref normal cl (RefFrom)
   var normalCLName = 'normal_34155';
   var cl = commCreateCL( db, COMMCSNAME, normalCLName, {
      AutoIncrement: { Field: "id" },
      Compressed: false,
      ConsistencyStrategy: 2,
      AutoIndexId: false,
      StrictDataMode: false } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var clName = 'cl_34159';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + normalCLName } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var normalCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + normalCLName } ).next().toObj();
   var clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   normalCLObj4Check = JSON.parse(JSON.stringify(normalCLObj));
   pickFieldAbleToRef( normalCLObj4Check );
   for (key in clObj)
   {
      assert.equal( normalCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // ref normal cl (RefObj)
   clName = 'cl_34155';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefObj: normalCLObj } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj );

   for (key in clObj)
   {
      assert.equal( normalCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );
   commDropCL( db, COMMCSNAME, normalCLName);

   var clName = 'cl_34163';
   var cappedCLName = 'normal_34163';
   var csName = 'cs_34163';
   commCreateCS( db, csName, false, "create CS capped", { Capped: true } );
   cl = commCreateCL( db, csName, cappedCLName, {
      Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false
      } );
   cl = commCreateCL( db, csName, clName, { RefFrom: csName + "." + cappedCLName } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   commDropCS( db, csName );
}

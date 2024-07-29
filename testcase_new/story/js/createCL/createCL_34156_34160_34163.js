/******************************************************************************
 * @Description   : seqDB-34156:RefObj 引用哈希分区表对象
 * @Description   : seqDB-34160:RefFrom 引用哈希分区表
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
   // ref hash cl (RefFrom)
   var hashCLName = 'hash_34156';
   var cl = commCreateCL( db, COMMCSNAME, hashCLName, {
      ShardingKey: { a: 1 },
      ShardingType: "hash",
      AutoSplit: true,
      AutoIncrement: [ { Field: "id1" }, { Field: "id2" } ],
      Compressed: false,
      ReplSize: 2,
      ConsistencyStrategy: 2,
      StrictDataMode: false } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var clName = 'cl_34160';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + hashCLName, RefMode: 2 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var hashCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + hashCLName } ).next().toObj();
   var clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   hashCLObj4Check = JSON.parse(JSON.stringify(hashCLObj));
   pickFieldAbleToRef( hashCLObj4Check );
   for (key in clObj)
   {
      assert.equal( hashCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // ref hash cl (RefObj)
   var clName = 'cl_34156';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefObj: hashCLObj, RefMode: 2 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj );

   for (key in clObj)
   {
      assert.equal( hashCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   commDropCL( db, COMMCSNAME, hashCLName);
}

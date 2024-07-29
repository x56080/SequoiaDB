/******************************************************************************
 * @Description   : seqDB-34157:RefObj 引用范围分区表对象
 * @Description   : seqDB-34161:RefFrom 引用范围分区表
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
   // ref range cl (RefFrom)
   var rangeCLName = 'range_34157';
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var srcGroup = groupsArray[0][0].GroupName;
   var cl = commCreateCL( db, COMMCSNAME, rangeCLName, {
      ShardingKey: { a: 1 },
      ShardingType: "range",
      Group: srcGroup,
      NoTrans: true,
      AutoIncrement: { Field: "id1", Increment: 2 },
      ConsistencyStrategy: 2,
      StrictDataMode: false } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   if (groupsArray.length > 1)
   {
      var dstGroup = groupsArray[1][0].GroupName;
      cl.split(srcGroup, dstGroup, 50);
   }

   var clName = 'cl_34161';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + rangeCLName, RefMode: 2 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var rangeCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + rangeCLName } ).next().toObj();
   var clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   rangeCLObj4Check = JSON.parse(JSON.stringify(rangeCLObj));
   pickFieldAbleToRef( rangeCLObj4Check );
   for (key in clObj)
   {
      assert.equal( rangeCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // ref range cl (RefObj)
   var clName = 'cl_34157';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefObj: rangeCLObj, RefMode: 2 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj );

   for (key in clObj)
   {
      assert.equal( rangeCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   commDropCL( db, COMMCSNAME, rangeCLName );
}

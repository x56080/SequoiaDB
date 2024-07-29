/******************************************************************************
 * @Description   : seqDB-34168:RefObj / RefFrom 引用范围分区表，RefMode 指定 0，SplitGroupStart 指定非 0 值
 * @Description   : seqDB-34169:RefObj / RefFrom 引用范围分区表，RefMode 指定 1，SplitGroupStart 指定非 0 值
 * @Description   : seqDB-34170:RefObj / RefFrom 引用范围分区表，RefMode 指定 2，SplitGroupStart 指定非 0 值
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
   var rangeCLName = 'range_34168';
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
   var dstGroup = groupsArray[1][0].GroupName;
   cl.split(srcGroup, dstGroup, 50);
   var rangeCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + rangeCLName } ).next().toObj();

   // range with RefMode 0
   clName = 'cl_34168';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + rangeCLName, RefMode: 0 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj );
   rangeCLObj4Check = JSON.parse(JSON.stringify(rangeCLObj));
   pickFieldAbleToRef( rangeCLObj4Check );
   // re-group exchange the groups but keep the same low / up bound
   for (var i = 0; i < clObj.CataInfo.length; i++)
   {
      var lowA = clObj.CataInfo[i].LowBound[ "" ];
      var upA = clObj.CataInfo[i].UpBound[ "" ];
      var found = false;
      for (var j = 0; j < rangeCLObj.CataInfo.length; j++)
      {
         var lowB = rangeCLObj.CataInfo[i].LowBound[ "" ];
         var upB = rangeCLObj.CataInfo[i].UpBound[ "" ];
         if (lowA == lowB && upA == upB)
         {
            found = true;
            break;
         }
      }
      assert.equal( found, true );
   }
   delete clObj.CataInfo;
   for (key in clObj)
   {
      assert.equal( rangeCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // range with RefMode 1
   clName = 'cl_34169';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + rangeCLName, RefMode: 1 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj );
   // re-group exchange the groups but keep the same low / up bound
   for (var i = 0; i < clObj.CataInfo.length; i++)
   {
      var lowA = clObj.CataInfo[i].LowBound[ "" ];
      var upA = clObj.CataInfo[i].UpBound[ "" ];
      var found = false;
      for (var j = 0; j < rangeCLObj.CataInfo.length; j++)
      {
         var lowB = rangeCLObj.CataInfo[i].LowBound[ "" ];
         var upB = rangeCLObj.CataInfo[i].UpBound[ "" ];
         if (lowA == lowB && upA == upB)
         {
            found = true;
            break;
         }
      }
      assert.equal( found, true );
   }
   delete clObj.CataInfo;
   for (key in clObj)
   {
      assert.equal( rangeCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // range with RefMode 2
   var clName = 'cl_34170';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + rangeCLName, RefMode: 2 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var rangeCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + rangeCLName } ).next().toObj();
   var clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   for (key in clObj)
   {
      assert.equal( rangeCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   commDropCL( db, COMMCSNAME, rangeCLName );
}

/******************************************************************************
 * @Description   : seqDB-34165:RefObj / RefFrom 引用哈希分区表，RefMode 指定 0，SplitGroupStart 指定非 0 值
 * @Description   : seqDB-34166:RefObj / RefFrom 引用哈希分区表，RefMode 指定 1，SplitGroupStart 指定非 0 值
 * @Description   : seqDB-34167:RefObj / RefFrom 引用哈希分区表，RefMode 指定 2，SplitGroupStart 指定非 0 值
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
   var hashCLName = 'hash_34165';
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
   var hashCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + hashCLName } ).next().toObj();

   // hash with RefMode 0
   var hashCL = db.getCS( COMMCSNAME ).getCL( hashCLName );
   // not balanced partitions make RefMode 0 and RefMode 1 different.
   srcGroup = hashCLObj.CataInfo[0].GroupName;
   dstGroup = hashCLObj.CataInfo[1].GroupName;
   srcGroupLow = hashCLObj.CataInfo[0].LowBound[ "" ];
   srcGroupUp = hashCLObj.CataInfo[0].UpBound[ "" ];
   hashCL.split( srcGroup, dstGroup, { Partition: srcGroupLow + 1 }, { Partition: srcGroupUp } );
   hashCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + hashCLName } ).next().toObj();

   var clName = 'cl_34165';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + hashCLName, RefMode: 0 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   // re-shard make it be balanced again
   for (var i = 0; i < clObj.CataInfo.length; i++)
   {
      var low = clObj.CataInfo[i].LowBound[ "" ];
      var up = clObj.CataInfo[i].UpBound[ "" ];
      assert.equal( up - low > 1, true );
   }
   delete clObj.CataInfo;
   hashCLObj4Check = JSON.parse(JSON.stringify(hashCLObj));
   pickFieldAbleToRef( hashCLObj4Check );
   for (key in clObj)
   {
      assert.equal( hashCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // hash with RefMode 1
   clName = 'cl_34166';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + hashCLName, RefMode: 1 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   pickFieldAbleToRef( clObj );
   // re-group exchange the groups but keep the same low / up bound
   for (var i = 0; i < clObj.CataInfo.length; i++)
   {
      var lowA = clObj.CataInfo[i].LowBound[ "" ];
      var upA = clObj.CataInfo[i].UpBound[ "" ];
      var found = false;
      for (var j = 0; j < hashCLObj.CataInfo.length; j++)
      {
         var lowB = hashCLObj.CataInfo[i].LowBound[ "" ];
         var upB = hashCLObj.CataInfo[i].UpBound[ "" ];
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
      assert.equal( hashCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   // hash with RefMode 2
   var clName = 'cl_34167';
   cl = commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + hashCLName, RefMode: 2 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   pickFieldAbleToRef( clObj );
   for (key in clObj)
   {
      assert.equal( hashCLObj4Check[ key ], clObj[ key ], key );
   }
   commDropCL( db, COMMCSNAME, clName );

   commDropCL( db, COMMCSNAME, hashCLName );
}

/******************************************************************************
*@Description : seqDB-34295:大量 overflow 数据回滚后 dropCL
*@author:      linsuqiang
*@createdate:  2025.03.05
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupName = groupsArray[0][0].GroupName;

   const csName = "cs_34295";
   const clName = "cl_34295";
   const totalRecords = 100000;
   const bulkSize = 1000;

   commDropCS( db, csName );

   var cs = commCreateCS( db, csName );
   var cl = commCreateCL( db, csName, clName, { Group: groupName, Compressed: false } );

   var array = [];
   for (var i = 0; i < totalRecords; i++)
   {
      array.push( { a: i, b: i, c: i } );
      if ( array.length >= bulkSize )
      {
         cl.insert( array );
         array = [];
      }
   }

   cl.update({ $set: { a: "this_is_a_long_long_string_to_make_record_overflow" } });
   cl.createIndex("aIdx", { a: 1 });

   db.transBegin();
   cl.remove();
   db.transRollback();

   commDropCL( db, csName, clName );



   const clName2 = "cl_34295_2";
   db.setSessionAttr({ TransMaxLockNum: -1 })
   var cl2 = commCreateCL( db, csName, clName2,
        { Group: groupName, ReplSize: -1, Compressed: false } );
   array = [];
   for (var i = 0; i < totalRecords; i++)
   {
      array.push( { a: i, b: "abcde", c: i * 5 } );
      if ( array.length >= bulkSize )
      {
         cl2.insert( array );
         array = [];
      }
   }

   db.transBegin();
   println( cl2.count({ _id: { $exists: true } }) );
   db.transCommit();

   db.transBegin();
   cl2.update(
        { $set: { d: "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk" } },
        { a: { $gte: 20000 } });
   db.transCommit();

   db.transBegin();
   cl2.remove({ a: { $gte: 2000, $lte: 65000 } });
   db.transRollback();

   cl2.count({ a: { $isnull: 0 }})

   db.setSessionAttr({ TransMaxLockNum: 10000 });
   commDropCS( db, csName );
}

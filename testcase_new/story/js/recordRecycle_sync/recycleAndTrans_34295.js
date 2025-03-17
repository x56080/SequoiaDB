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
   var cl = commCreateCL( db, csName, clName, { Group: groupName } );

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

   commDropCS( db, csName );
}

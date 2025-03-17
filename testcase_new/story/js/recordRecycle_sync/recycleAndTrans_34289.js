/******************************************************************************
*@Description : seqDB-34289:多事务删除并发回滚/提交
*@author:      linsuqiang
*@createdate:  2025.03.05
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupName = groupsArray[0][0].GroupName;

   const csName = "cs_34289";
   const clName = "cl_34289";
   const totalRecords = 1000;

   commDropCS( db, csName );

   var cs = commCreateCS( db, csName );
   commCreateCL( db, csName, clName, { Group: groupName } );

   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var db3 = new Sdb( COORDHOSTNAME, COORDSVCNAME );

   // head
   db.getCS( csName ).getCL( clName ).insert([
       { a: 1, b: 1, c: 1},
       { a: 2, b: 2, c: 2},
       { a: 3, b: 3, c: 3}
   ])
   db1.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 1 });

   db2.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 2 });

   db3.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 3 });

   db2.transCommit()
   db3.transCommit()
   db1.transRollback()

   // middle
   db.getCS( csName ).getCL( clName ).insert([
       { a: 1, b: 1, c: 1},
       { a: 2, b: 2, c: 2},
       { a: 3, b: 3, c: 3}
   ])
   db1.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 1 });

   db2.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 2 });

   db3.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 3 });

   db1.transCommit()
   db3.transCommit()
   db2.transRollback()

   // tail
   db.getCS( csName ).getCL( clName ).insert([
       { a: 1, b: 1, c: 1},
       { a: 2, b: 2, c: 2},
       { a: 3, b: 3, c: 3}
   ])
   db1.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 1 });

   db2.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 2 });

   db3.transBegin()
   db1.getCS( csName ).getCL( clName ).remove({ a: 3 });

   db1.transCommit()
   db2.transCommit()
   db3.transRollback()

   commDropCS( db, csName );
}

/******************************************************************************
 * @Description   : seqDB-32928:renameCollection接口功能验证
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName_1 = "cl_32928_1";
   var clName_2 = "cl_32928_2";
   var cl_1 = db.getCollection( clName_1 );
   var cl_2 = db.getCollection( clName_2 );
   cl_1.drop();
   cl_2.drop();

   // rename，源表不存在
   var rc = cl_1.renameCollection( clName_1 );
   assert.eq( rc.ok, 0 );

   // rename，源表存在，目标表名不存在
   // 目标名跟原名相同
   cl_1.insert( { "_id": 1 } );
   var rc = cl_1.renameCollection( clName_1 );
   assert.eq( rc.ok, 0 );
   assert.eq( rc.code, -22 );
   checkResults( cl_1.find(), "[{\"_id\":1}]" );
   // 目标名跟原名不同
   var rc = cl_1.renameCollection( clName_2 );
   assert.eq( rc, { "ok": 1 } );
   checkResults( cl_1.find(), "[]" );
   checkResults( cl_2.find(), "[{\"_id\":1}]" );


   // rename，dropTarget参数测试
   cl_1.drop();
   cl_2.drop();
   cl_1.insert( { "_id": 1 } );
   cl_2.insert( { "_id": 2 } );

   // dropTarget:false
   var rc = cl_1.renameCollection( clName_2, false );
   assert.eq( rc.ok, 0 );
   assert.eq( rc.code, -22 );
   checkResults( cl_1.find(), "[{\"_id\":1}]" );
   checkResults( cl_2.find(), "[{\"_id\":2}]" );

   // dropTarget:true（ 为true时会先删除已存在的CL ），fapmongo 不支持
   var rc = cl_1.renameCollection( clName_2, true );
   // SEQUOIADBMAINSTREAM-9898，非问题
   assert.eq( rc.code, -22 );


   // db.runCommand(...renameCollection...)
   cl_1.drop();
   cl_2.drop();
   cl_1.insert( { "_id": 1 } );
   // 集合空间和集合名均被rename
   var rc = db.adminCommand( { "renameCollection": cl_1.getFullName(), "to": cl_2.getFullName(), "dropTarget": false, "writeConcern": { "j": true } } );
   assert.eq( rc, { "ok": 1 } );
   checkResults( cl_1.find(), "[]" );
   checkResults( cl_2.find(), "[{\"_id\":1}]" );

   cl_1.drop();
   cl_2.drop();
}

function checkResults ( cursor, expDocs )
{
   var docs = new Array();
   while( cursor.hasNext() )
   {
      var doc = cursor.next();
      docs.push( doc );
   }
   cursor.close();
   assert.eq( JSON.stringify( docs ), expDocs );
}
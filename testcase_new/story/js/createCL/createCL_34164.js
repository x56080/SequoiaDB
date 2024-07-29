/******************************************************************************
 * @Description   : seqDB-34164:使用 RefObj / RefFrom 选项同时指定与源表不同的非分区选项
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var normalCLName = 'normal_34164';
   var cl = commCreateCL( db, COMMCSNAME, normalCLName, {
      AutoIncrement: { Field: "id" },
      Compressed: false,
      ConsistencyStrategy: 2,
      AutoIndexId: false,
      StrictDataMode: false } );
   var normalCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + normalCLName } ).next().toObj();

   var clName = 'cl_34164';
   var options = {
      RefFrom: COMMCSNAME + "." + normalCLName,
      AutoIncrement: { Field: "id2" },
      Compressed: false,
      NoTrans: true,
      ConsistencyStrategy: 1,
      AutoIndexId: true,
      StrictDataMode: true
   };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);

   var clObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();

   assert.equal( clObj.ConsistencyStrategy, 1 );
   assert.equal( clObj.AttributeDesc, "StrictDataMode | NoTrans" );
   assert.equal( clObj.CataInfo, normalCLObj.CataInfo );

   commDropCL( db, COMMCSNAME, clName );
   commDropCL( db, COMMCSNAME, normalCLName );
}

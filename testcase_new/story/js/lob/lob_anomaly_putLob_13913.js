/******************************************************************************
*@Description : anomaly test that for putLob/listLob/deleteLob
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

main( test );

function test ()
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   //lobGenerateFile( testFile ); // auto file
   // create collection
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // put lob with no lob file[Test_Point_1]
   assert.tryThrow( -6, function()
   {
      cl.listLobs( testFile );
   } );

   assert.tryThrow( -259, function()
   {
      cl.putLob();
   } );

   // delete lob with no oid[Test_Point_2]
   assert.tryThrow( -259, function()
   {
      cl.deleteLob();
   } );
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

}

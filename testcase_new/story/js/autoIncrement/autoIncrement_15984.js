/******************************************************************************
@Description :   seqDB-15984:  新增自增字段与已存在的自增字段同名
@Modify list :   2018-10-16    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15984";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1" } } );

   //create same autoIncrement field name
   assert.tryThrow( -332, function()
   {
      dbcl.createAutoIncrement( { Field: "id1" } );
   } );

   commDropCL( db, COMMCSNAME, clName );
}


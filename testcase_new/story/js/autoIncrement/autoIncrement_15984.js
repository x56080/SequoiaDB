/******************************************************************************
@Description :   seqDB-15984:  新增自增字段与已存在的自增字段同名
@Modify list :   2018-10-16    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15984";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1" } } );

   //create same autoIncrement field name
   try
   {
      dbcl.createAutoIncrement( { Field: "id1" } );
      throw "create autoIncrement error";
   } catch( e )
   {
      if( e !== -332 )
      {
         throw new Error( e );
      }
   }

   commDropCL( db, COMMCSNAME, clName );
}

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

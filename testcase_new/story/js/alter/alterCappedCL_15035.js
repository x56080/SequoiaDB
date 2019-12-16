/* *****************************************************************************
@discretion: cappedcl alter size/max/overWrite, the test scenario is as follows:
test a: alter size/max/overWrite
test b: alter capped
@author��2018-4-27 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15035";
var clName = CHANGEDPREFIX + "_altercappedcl_15035";

try
{
   main( db );
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ( db )
{
   try
   {
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      //clean environment before test
      commDropCS( db, csName, true, "drop cs" );

      //create cappedCS
      commCreateCS( db, csName, false, "beginning to create cappedCS", { Capped: true } );
      var dbcl = commCreateCL( db, csName, clName, { Capped: true, Size: 1024, Max: 10000, AutoIndexId: false } );

      //test a :alter size/max/overWrite
      println( "---alter capped cl size/max/overWrite" );
      var size = 2048;
      var max = 8000;
      var overWrite = true;
      dbcl.alter( { Size: size, Max: max, OverWrite: overWrite } );
      checkAlterCappedCLResult( clName, max, size * 1024 * 1024, overWrite );

      //test b: alter
      println( "---alter capped cl capped" );
      alterCapped( dbcl );

      //clean
      commDropCS( db, csName, true, "clear cs" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

function alterCapped ( dbcl )
{
   try
   {
      dbcl.alter( { Capped: false } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }
}

function checkAlterCappedCLResult ( clName, expMax, expSize, expOverWrite )
{
   var clFullName = csName + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } ).current().toObj();
   var actMax = cur["Max"];
   var actSize = cur["Size"];
   var actOverWrite = cur["OverWrite"];

   if( expMax !== actMax && expSize !== actSize && expOverWrite !== actOverWrite )
   {
      throw buildException( "test value", "check field", "", "",
         "actMax=" + actMax + " actSize=" + actSize + " actOverWrite=" + actOverWrite );
   }

}


/******************************************************************************
@Description : listCL basic: listCollections

@Modify list :
2015-01-29 pusheng Ding  Init
******************************************************************************/

csName = CHANGEDPREFIX + "_foo";
clName_1 = CHANGEDPREFIX + "_bar1";
clName_2 = CHANGEDPREFIX + "_bar2";
var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

try
{
   commDropCL( db, csName, clName_1, true, true, "drop cl1 in begin" );
   commDropCL( db, csName, clName_2, true, true, "drop cl2 in begin" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var varCS = commCreateCS( db, csName, true, "create CS" );
}
catch( e )
{
   println( "failed to create cs, rc= " + e )
   throw e;
}

try
{
   var varCL = varCS.createCL( clName_1, { ReplSize: 0, Compressed: true } );
}
catch( e )
{
   println( "failed to create cl, rc= " + e )
   throw e;
}

try
{
   var varCL1 = varCS.createCL( clName_2, { ReplSize: 0, Compressed: true } );
}
catch( e )
{
   println( "failed to create cl, rc= " + e )
   throw e;
}


try
{
   var cur = db.listCollections();
}
catch( e )
{
   println( "failed to list collections, rc= " + e );
   throw e;
}

while( cur.next() )
{

   var strCSAndCLName = cur.current().toObj()["Name"]
   var strCS = strCSAndCLName.substring( 0, strCSAndCLName.indexOf( "." ) );
   var strCL = strCSAndCLName.substring( strCSAndCLName.indexOf( "." ) + 1 );

   if( strCS == csName && ( strCL != clName_1 && strCL != clName_2 ) )
   {
      println( "expect cs: " + csName + ", actual: " + strCS );
      println( "expect cl1: " + clName_1 + ", actual: " + strCL );
      println( "expect cl2: " + clName_2 + ", actual: " + strCL );
      println( "uncorrect name in list: " + cur.current().toObj()["Name"] )
      throw -1;
   }
}

try
{
   varCS.dropCL( clName_1 );
}
catch( e )
{
   println( "failed to drop collections, rc= " + e );
   throw e;
}

try
{
   cur = db.listCollections();
}
catch( e )
{
   println( "failed to list collections, rc= " + e );
   throw e;
}


while( cur.next() )
{


   var strCSAndCLName = cur.current().toObj()["Name"];
   var strCS = strCSAndCLName.substring( 0, strCSAndCLName.indexOf( "." ) );
   var strCL = strCSAndCLName.substring( strCSAndCLName.indexOf( "." ) + 1 );

   if( strCS == csName && strCL != clName_2 )
   {
      println( "uncorrect name in list: " + o["Name"] )
      throw -1;
   }
}

try
{
   commDropCL( db, csName, clName_1, true, true, "drop cl1 in end" );
   commDropCL( db, csName, clName_2, false, false, "drop cl2 in end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

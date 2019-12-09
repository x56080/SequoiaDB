/****************************************************
@description:	create/drop CL by SQL, verify parameters
         testlink cases:   seqDB-7411/7412
@input:        1 create/drop cl, the length of the name is 127B (valid length), success
               2 create/drop cl, the length of the name is 128B (invalid length), errorno: -6 
               3 create/drop cl, the name is " ", errorno: -6
               4 create/drop cl, the name cotains special characters, errorno: -6
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName = CHANGEDPREFIX + "_bar";

println( "------Begin to clean env in the begin." );
try  //clean cl
{
   db.execUpdate( "drop collection " + csName + "." + clName );
}
catch( e )
{
   if( e != -23 )
   {
      println( "Failed to clean env in the begin." );
      throw e;
   }
}

println( "------Begin to create cl,the length of the name is 127B (valid length)." );
var specCLName1 = "";
for( var i = 0; i < 127; ++i )
{
   specCLName1 = specCLName1 + "a";
}

try  //clean cl
{
   db.execUpdate( "drop collection " + csName + "." + specCLName1 );
}
catch( e )
{
   if( e != -23 )
   {
      println( "Failed to clean env " + csName + "." + specCLName1 );
      throw e;
   }
}

try //create cl
{
   db.execUpdate( "create collection " + csName + "." + specCLName1 );
}
catch( e )
{
   println( "Failed to create cl, the length of the name is 127B. Except result is success." );
   throw e;
}

println( "------Begin to check results." );
try
{
   db.getCS( csName ).getCL( specCLName1 );
}
catch( e )
{
   if( e !== 23 )
   {
      println( "Failed to drop cl. rc=3" );
      throw e;
   }
}

println( "------Begin to drop the cl." );
try  
{
   db.execUpdate( "drop collection " + csName + "." + specCLName1 );
}
catch( e )
{

   println( "Failed to drop CL." );
   throw e;
}

println( "------Begin to create CL,the length of the name is 128B (invalid length)." );
var specCLName2 = "";
for( var i = 0; i < 128; ++i )
{
   specCLName2 = specCLName2 + "a";
}

try
{
   db.execUpdate( "create collection " + csName + "." + specCLName2 );
   db.execUpdate( "drop collection " + csName + "." + specCLName2 );
}
catch( e )
{
   if( e !== -6 )
      throw "The length of the name is 128B (invalid length),create cl success. Except errorno: -6";
}

println( '------Begin to create/drop cl, the name is " ".' );
try
{
   //create cl
   db.execUpdate( "create collection " + csName + "." + " " );
   //drop cl
   db.execUpdate( "drop collection " + csName + "." + " " );
}
catch( e )
{
   if( e !== -6 )
   {
      throw e;
   }
}

println( "------Begin to create cl, the name cotains invalid characters." );
var aa = Array( "$", ".", "a." );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var specCLName3 = aa[i] + CHANGEDPREFIX;
      //create cl
      db.execUpdate( "create collection " + csName + "." + specCLName3 );
      //drop cl
      db.execUpdate( "drop collection " + csName + "." + specCLName3 );
      //expect errorno: -6 ,if create success then throw
      throw "The name contains invalid characters,create cl success. Expect errorno: -6 ";
   } catch( e )
   {
      if( e !== -6 )
         throw e;
   }
}
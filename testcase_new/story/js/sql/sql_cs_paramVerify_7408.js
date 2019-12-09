/****************************************************
@description:	create/drop CS by SQL, verify parameters
         testlink cases:   seqDB-7408/7409
@input:        1 create cs, the name is " ", errorno: -195
               2 create cs, the name cotains special characters, errorno: -6
               3 create/drop cs, the length of the name is 127B (valid length), success
               4 create cs, the length of the name is 128B (invalid length), errorno: -6
               5 drop cs, the name contains invalid characters, errorno: -6 or -34
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
var csName = CHANGEDPREFIX + "_foo";
var aa = Array( "$", ".", "a." );

println( "------Begin to clean env in the begin." );
try
{
   db.execUpdate( "drop collectionspace " + csName );
}
catch( e )
{
   if( e != -34 )
   {
      println( "Failed to clean env in the begin." );
      throw e;
   }
}

for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var specCSName = aa[i] + CHANGEDPREFIX;
      db.execUpdate( "drop collectionspace " + specCSName );
   }
   catch( e ) { }
}

println( '------Begin to create cs, the name is " ".' );
try
{
   db.execUpdate( "create collectionspace " + " " );
}
catch( e )
{
   if( e !== -195 )
   {
      throw e;
   }
}

println( "------Begin to create cs,the length of the name is 127B (valid length)." );
var addLen1 = 127 - csName.length;
for( var i = 0; i < addLen1; ++i )
{
   csName = csName + "a";
}

try //clean the cs
{
   db.dropCS( csName );
}
catch( e )
{
   if( e != -34 )
   {
      throw e;
   }
}

try  //create the cs
{
   db.execUpdate( "create collectionspace " + csName );
}
catch( e )
{
   println( "Failed to create cs, the length of the name is 127B. Except result is success." );
   throw e;
}

println( "------Begin to drop the cs." )
try
{
   db.execUpdate( "drop collectionspace " + csName );
}
catch( e )
{
   println( "Failed to drop cs, the length of the name is 127B. Except result is success." );
   throw e;
}

println( "------Begin to create cs,the length of the name is 128B (invalid length)." );
var addLen2 = 128 - csName.length;
for( var i = 0; i < addLen2; ++i )
{
   csName = csName + "a";
}

try
{
   db.execUpdate( "create collectionspace " + csName );
   throw "The length of the name is 128B (invalid length),create cs success. Except errorno: -6";
}
catch( e )
{
   if( e !== -6 )
      throw e;
}

println( "------Begin to create cs, the name cotains invalid characters." );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var specCSName = aa[i] + CHANGEDPREFIX;
      db.execUpdate( "create collectionspace " + specCSName );
      //expect errorno: -6 ,if create success then throw
      throw "The name contains invalid characters,create cs success. Expect errorno: -6 ";
   } catch( e )
   {
      if( e !== -6 )
         throw e;
   }
}

println( "------Begin to drop cs, the name contains invalid characters." );
for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var specCSName = aa[i] + CHANGEDPREFIX;
      db.execUpdate( "drop collectionspace " + specCSName );
   }
   catch( e )
   {
      if( e !== -34 && e !== -6 )
      {
         throw e;
      }
   }
}
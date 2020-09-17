/* *****************************************************************************
@Description : Insert data that field name have special character.Such as
               " ~!@#$%^&*()_+}|{ ". The $ and . character are limited in SDB.
@Modify list :
               2014-6-4  xiaojun Hu  Init
***************************************************************************** */

main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_12136";
   var varCL = readyCL( clName );

   // special character put in array
   var specialStr = "~ ` ! @ # $ % ^ * ( ) - _ = + { } [ ] | \\ : ; \" < > ? / . $";
   var specialChar = new Array();
   specialChar = specialStr.split( " " );

   // get the collection
   var cs = db.getCS( csName );
   var cl = cs.getCL( clName );

   // insert the data
   for( var i = 0; i < specialChar.length; ++i )
   {
      insertInspectStr( cl, specialChar[i], "insert" )
   }

   // inspect the data
   for( var i = 0; i < specialChar.length; ++i )
   {
      insertInspectStr( cl, specialChar[i], "inspect" )
   }

   // clean - drop cl
   commDropCL( db, csName, clName, false, false, "Failed to drop CL in the end-condition" );
}

/*********************************************************************************
@Description : Insert special characters in field name, such as "
               ~!<>?:"{}|+_)(*&^%$#@ ".
@Author :      xiaojun Hu
@Parameter :
               key: Field name include special characters.
               value: common string type field value.
**********************************************************************************/
function insertInspectStr ( cl, spekey, mode )
{
   try
   {
      if( cl == "" || cl == undefined ||
         spekey == "" || spekey == undefined ||
         mode == "" || mode == undefined )
      {
         throw new Error( "cl: " + cl + "\nspekey: " + spekey + "\nmode: " + mode );
      }
      var insertStr = "{'" + spekey + "key" + spekey + "field':'SpecialCharacterValue'}";
      var evalInsertStr = eval( "(" + insertStr + ")" );
      if( "insert" == mode )
         cl.insert( evalInsertStr );
      else if( "inspect" == mode )
      {
         var query = cl.find( evalInsertStr ).count();
         if( 1 != query && "$" != spekey && "." != spekey )
         {
            throw new Error( "query: " + query + "\nspekey: " + spekey );
         }
      }
   }
   catch( e )
   {
      if( -6 != e.message )
      {
         throw e;
      }
   }
}

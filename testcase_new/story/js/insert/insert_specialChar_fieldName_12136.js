/* *****************************************************************************
@Description : Insert data that field name have special character.Such as
               " ~!@#$%^&*()_+}|{ ". The $ and . character are limited in SDB.
@Modify list :
               2014-6-4  xiaojun Hu  Init
***************************************************************************** */

var csName = COMMCSNAME;
var clName = COMMCLNAME;

function main ( db )
{
   // drop cl
   commDropCL( db, csName, clName, true, true, "drop collection in begin" );

   // create cl
   var varCL = commCreateCL( db, csName, clName, -1, false, true, false,
      "create collection in begin" );
   // special character put in array
   var specialStr = "~ ` ! @ # $ % ^ * ( ) - _ = + { } [ ] | \\ : ; \" < > ? / . $";
   var specialChar = new Array();
   specialChar = specialStr.split( " " );
   //println( specialChar ) ;

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
   commDropCS( db, csName, "drop CS in the end" )
}

// main entry
try
{
   main( db );
   db.close();
}
catch( e )
{
   throw e;
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
         println( "Wrong argument." );
         throw "ErrArg";
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
            println( "Failed to inspect the data , special char = " + spekey );
            throw "ErrSpeChar";
         }
      }
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "Failed to insert special character." + e );
         throw e;
      }
   }
}


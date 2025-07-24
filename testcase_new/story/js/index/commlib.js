/******************************************************************************
 * @Description   : Create Index common functions
 * @Author        : Wu Yan
 * @CreateTime    : 2021.12.17
 * @LastEditTime  : 2021.12.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
import( "../lib/index_commlib.js" );
import( "../lib/basic_operation/commlib.js" );


<<<<<<< HEAD
function getOneSample ( db, csName, clName, ixName )
{
   var cl = db.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );
   var stat = cl.find( { CollectionSpace: csName, Collection: clName, Index: ixName } ).current().toObj();
   return stat.MCV.Values[1];
}
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

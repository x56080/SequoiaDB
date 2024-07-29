import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function pickFieldAbleToRef( catalog, isMainCL )
{
   delete catalog._id;
   delete catalog.Name;
   delete catalog.UniqueID;
   delete catalog.Version;
   delete catalog.CreateTime;
   delete catalog.UpdateTime;
   if (isMainCL)
   {
      delete catalog.CataInfo;
   }
   if (catalog.AutoIncrement)
   {
      for (var i = 0; i < catalog.AutoIncrement.length; i++)
      {
         delete catalog.AutoIncrement[i].SequenceName;
         delete catalog.AutoIncrement[i].SequenceID;
      }
   }
}

#include <render/Culling/CullData.h>

CullCache& theCullCache()
{
   //
   // This will eventually house the ScratchArray implementation.
   // In the meantime, we'll use a placeholder
   //
   static CullCache tcc;

   return tcc;
}

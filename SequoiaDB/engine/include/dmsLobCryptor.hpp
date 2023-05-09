/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = dmsLobEncryptor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/30/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilELCryptor.hpp"
#include "ossGMCrypto.hpp"

namespace engine
{
   class _dmsLobCryptor : public utilELCryptor
   {
      public:
         _dmsLobCryptor( ossSM4Key dek, UINT8 *nonce, UINT32 offset ) ;
         virtual INT32 encrypt( const UINT8 *in,
                                UINT32 ilen,
                                UINT8 *out,
                                UINT32 *olen = NULL ) const ;
         
         virtual INT32 decrypt( const UINT8 *in,
                                UINT32 ilen,
                                UINT8 *out,
                                UINT32 *olen = NULL ) const ;

      private:
         ossSM4Key _dek ;
         ossSM4Vec _ctr ;
         UINT32 _offset ;
   } ;
   typedef _dmsLobCryptor dmsLobCryptor ;
} // namespace engine
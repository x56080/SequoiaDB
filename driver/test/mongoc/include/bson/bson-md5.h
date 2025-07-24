/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   
*******************************************************************************/
/* $Id: md5.h,v 1.4 2002/04/13 19:20:28 lpd Exp $ */
/*
  Independent implementation of MD5 (RFC 1321).

  This code implements the MD5 Algorithm defined in RFC 1321, whose
  text is available at
    http://www.ietf.org/rfc/rfc1321.txt
  The code is derived from the text of the RFC, including the test suite
  (section A.5) but excluding the rest of Appendix A.  It does not include
  any code or documentation that is identified in the RFC as being
  copyrighted.

  The original and principal author of md5.h is L. Peter Deutsch
  <ghost@aladdin.com>.  Other authors are noted in the change history
  that follows (in reverse chronological order):

  2002-04-13 lpd Removed support for non-ANSI compilers; removed
    references to Ghostscript; clarified derivation from RFC 1321;
    now handles byte order either statically or dynamically.
  1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
  1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5);
    added conditionalization for C++ compilation from Martin
    Purschke <purschke@bnl.gov>.
  1999-05-03 lpd Original version.
 */


/*
 * The following MD5 implementation has been modified to use types as
 * specified in libbson.
 */


#ifndef BSON_MD5_H
#define BSON_MD5_H


#if !defined (BSON_INSIDE) && !defined (BSON_COMPILATION)
#  error "Only <bson.h> can be included directly."
#endif


#include "bson-endian.h"


BSON_BEGIN_DECLS


typedef struct
{
   uint32_t count[2]; /* message length in bits, lsw first */
   uint32_t abcd[4];  /* digest buffer */
   uint8_t  buf[64];  /* accumulate block */
} bson_md5_t;


void bson_md5_init   (bson_md5_t         *pms);
void bson_md5_append (bson_md5_t         *pms,
                      const uint8_t *data,
                      uint32_t       nbytes);
void bson_md5_finish (bson_md5_t         *pms,
                      uint8_t        digest[16]);


BSON_END_DECLS


#endif /* BSON_MD5_H */

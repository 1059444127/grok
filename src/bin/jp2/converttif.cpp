/*
*    Copyright (C) 2016 Grok Image Compression Inc.
*
*    This source code is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This source code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*
*    This source code incorporates work covered by the following copyright and
*    permission notice:
*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * Copyright (c) 2015, Matthieu Darbois
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

extern "C" {
#include "opj_apps_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef OPJ_HAVE_LIBTIFF
# error OPJ_HAVE_LIBTIFF_NOT_DEFINED
#endif /* OPJ_HAVE_LIBTIFF */

#include <tiffio.h>
#include "openjpeg.h"
#include "convert.h"
}
/* -->> -->> -->> -->>

 TIFF IMAGE FORMAT

 <<-- <<-- <<-- <<-- */
#define PUTBITS2(s, nb) \
	trailing <<= remaining; \
	trailing |= (unsigned int)((s) >> (nb - remaining)); \
	*pDst++ = (OPJ_BYTE)trailing; \
	trailing = (unsigned int)((s) & ((1U << (nb - remaining)) - 1U)); \
	if (nb >= (remaining + 8)) { \
		*pDst++ = (OPJ_BYTE)(trailing >> (nb - (remaining + 8))); \
		trailing &= (unsigned int)((1U << (nb - (remaining + 8))) - 1U); \
		remaining += 16 - nb; \
	} else { \
		remaining += 8 - nb; \
	}

#define PUTBITS(s, nb) \
  if (nb >= remaining) { \
		PUTBITS2(s, nb) \
	} else { \
		trailing <<= nb; \
		trailing |= (unsigned int)(s); \
		remaining -= nb; \
	}
#define FLUSHBITS() \
	if (remaining != 8) { \
		trailing <<= remaining; \
		*pDst++ = (OPJ_BYTE)trailing; \
	}

static void tif_32sto3u(const int32_t* pSrc, OPJ_BYTE* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;

	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t src0 = (uint32_t)pSrc[i + 0];
		uint32_t src1 = (uint32_t)pSrc[i + 1];
		uint32_t src2 = (uint32_t)pSrc[i + 2];
		uint32_t src3 = (uint32_t)pSrc[i + 3];
		uint32_t src4 = (uint32_t)pSrc[i + 4];
		uint32_t src5 = (uint32_t)pSrc[i + 5];
		uint32_t src6 = (uint32_t)pSrc[i + 6];
		uint32_t src7 = (uint32_t)pSrc[i + 7];

		*pDst++ = (OPJ_BYTE)((src0 << 5) | (src1 << 2) | (src2 >> 1));
		*pDst++ = (OPJ_BYTE)((src2 << 7) | (src3 << 4) | (src4 << 1) | (src5 >> 2));
		*pDst++ = (OPJ_BYTE)((src5 << 6) | (src6 << 3) | (src7));
	}

	if (length & 7U) {
		unsigned int trailing = 0U;
		int remaining = 8U;
		length &= 7U;
		PUTBITS((uint32_t)pSrc[i + 0], 3)
			if (length > 1U) {
				PUTBITS((uint32_t)pSrc[i + 1], 3)
					if (length > 2U) {
						PUTBITS((uint32_t)pSrc[i + 2], 3)
							if (length > 3U) {
								PUTBITS((uint32_t)pSrc[i + 3], 3)
									if (length > 4U) {
										PUTBITS((uint32_t)pSrc[i + 4], 3)
											if (length > 5U) {
												PUTBITS((uint32_t)pSrc[i + 5], 3)
													if (length > 6U) {
														PUTBITS((uint32_t)pSrc[i + 6], 3)
													}
											}
									}
							}
					}
			}
		FLUSHBITS()
	}
}

static void tif_32sto5u(const int32_t* pSrc, OPJ_BYTE* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;

	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t src0 = (uint32_t)pSrc[i + 0];
		uint32_t src1 = (uint32_t)pSrc[i + 1];
		uint32_t src2 = (uint32_t)pSrc[i + 2];
		uint32_t src3 = (uint32_t)pSrc[i + 3];
		uint32_t src4 = (uint32_t)pSrc[i + 4];
		uint32_t src5 = (uint32_t)pSrc[i + 5];
		uint32_t src6 = (uint32_t)pSrc[i + 6];
		uint32_t src7 = (uint32_t)pSrc[i + 7];

		*pDst++ = (OPJ_BYTE)((src0 << 3) | (src1 >> 2));
		*pDst++ = (OPJ_BYTE)((src1 << 6) | (src2 << 1) | (src3 >> 4));
		*pDst++ = (OPJ_BYTE)((src3 << 4) | (src4 >> 1));
		*pDst++ = (OPJ_BYTE)((src4 << 7) | (src5 << 2) | (src6 >> 3));
		*pDst++ = (OPJ_BYTE)((src6 << 5) | (src7));

	}

	if (length & 7U) {
		unsigned int trailing = 0U;
		int remaining = 8U;
		length &= 7U;
		PUTBITS((uint32_t)pSrc[i + 0], 5)
			if (length > 1U) {
				PUTBITS((uint32_t)pSrc[i + 1], 5)
					if (length > 2U) {
						PUTBITS((uint32_t)pSrc[i + 2], 5)
							if (length > 3U) {
								PUTBITS((uint32_t)pSrc[i + 3], 5)
									if (length > 4U) {
										PUTBITS((uint32_t)pSrc[i + 4], 5)
											if (length > 5U) {
												PUTBITS((uint32_t)pSrc[i + 5], 5)
													if (length > 6U) {
														PUTBITS((uint32_t)pSrc[i + 6], 5)
													}
											}
									}
							}
					}
			}
		FLUSHBITS()
	}
}

static void tif_32sto7u(const int32_t* pSrc, OPJ_BYTE* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;

	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t src0 = (uint32_t)pSrc[i + 0];
		uint32_t src1 = (uint32_t)pSrc[i + 1];
		uint32_t src2 = (uint32_t)pSrc[i + 2];
		uint32_t src3 = (uint32_t)pSrc[i + 3];
		uint32_t src4 = (uint32_t)pSrc[i + 4];
		uint32_t src5 = (uint32_t)pSrc[i + 5];
		uint32_t src6 = (uint32_t)pSrc[i + 6];
		uint32_t src7 = (uint32_t)pSrc[i + 7];

		*pDst++ = (OPJ_BYTE)((src0 << 1) | (src1 >> 6));
		*pDst++ = (OPJ_BYTE)((src1 << 2) | (src2 >> 5));
		*pDst++ = (OPJ_BYTE)((src2 << 3) | (src3 >> 4));
		*pDst++ = (OPJ_BYTE)((src3 << 4) | (src4 >> 3));
		*pDst++ = (OPJ_BYTE)((src4 << 5) | (src5 >> 2));
		*pDst++ = (OPJ_BYTE)((src5 << 6) | (src6 >> 1));
		*pDst++ = (OPJ_BYTE)((src6 << 7) | (src7));
	}

	if (length & 7U) {
		unsigned int trailing = 0U;
		int remaining = 8U;
		length &= 7U;
		PUTBITS((uint32_t)pSrc[i + 0], 7)
			if (length > 1U) {
				PUTBITS((uint32_t)pSrc[i + 1], 7)
					if (length > 2U) {
						PUTBITS((uint32_t)pSrc[i + 2], 7)
							if (length > 3U) {
								PUTBITS((uint32_t)pSrc[i + 3], 7)
									if (length > 4U) {
										PUTBITS((uint32_t)pSrc[i + 4], 7)
											if (length > 5U) {
												PUTBITS((uint32_t)pSrc[i + 5], 7)
													if (length > 6U) {
														PUTBITS((uint32_t)pSrc[i + 6], 7)
													}
											}
									}
							}
					}
			}
		FLUSHBITS()
	}
}

static void tif_32sto9u(const int32_t* pSrc, OPJ_BYTE* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;

	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t src0 = (uint32_t)pSrc[i + 0];
		uint32_t src1 = (uint32_t)pSrc[i + 1];
		uint32_t src2 = (uint32_t)pSrc[i + 2];
		uint32_t src3 = (uint32_t)pSrc[i + 3];
		uint32_t src4 = (uint32_t)pSrc[i + 4];
		uint32_t src5 = (uint32_t)pSrc[i + 5];
		uint32_t src6 = (uint32_t)pSrc[i + 6];
		uint32_t src7 = (uint32_t)pSrc[i + 7];

		*pDst++ = (OPJ_BYTE)((src0 >> 1));
		*pDst++ = (OPJ_BYTE)((src0 << 7) | (src1 >> 2));
		*pDst++ = (OPJ_BYTE)((src1 << 6) | (src2 >> 3));
		*pDst++ = (OPJ_BYTE)((src2 << 5) | (src3 >> 4));
		*pDst++ = (OPJ_BYTE)((src3 << 4) | (src4 >> 5));
		*pDst++ = (OPJ_BYTE)((src4 << 3) | (src5 >> 6));
		*pDst++ = (OPJ_BYTE)((src5 << 2) | (src6 >> 7));
		*pDst++ = (OPJ_BYTE)((src6 << 1) | (src7 >> 8));
		*pDst++ = (OPJ_BYTE)(src7);
	}

	if (length & 7U) {
		unsigned int trailing = 0U;
		int remaining = 8U;
		length &= 7U;
		PUTBITS2((uint32_t)pSrc[i + 0], 9)
			if (length > 1U) {
				PUTBITS2((uint32_t)pSrc[i + 1], 9)
					if (length > 2U) {
						PUTBITS2((uint32_t)pSrc[i + 2], 9)
							if (length > 3U) {
								PUTBITS2((uint32_t)pSrc[i + 3], 9)
									if (length > 4U) {
										PUTBITS2((uint32_t)pSrc[i + 4], 9)
											if (length > 5U) {
												PUTBITS2((uint32_t)pSrc[i + 5], 9)
													if (length > 6U) {
														PUTBITS2((uint32_t)pSrc[i + 6], 9)
													}
											}
									}
							}
					}
			}
		FLUSHBITS()
	}
}



static void tif_32sto10u(const int32_t* pSrc, uint8_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < (length & ~(size_t)3U); i+=4U) {
        uint32_t src0 = (uint32_t)pSrc[i+0];
        uint32_t src1 = (uint32_t)pSrc[i+1];
        uint32_t src2 = (uint32_t)pSrc[i+2];
        uint32_t src3 = (uint32_t)pSrc[i+3];

        *pDst++ = (uint8_t)(src0 >> 2);
        *pDst++ = (uint8_t)(((src0 & 0x3U) << 6) | (src1 >> 4));
        *pDst++ = (uint8_t)(((src1 & 0xFU) << 4) | (src2 >> 6));
        *pDst++ = (uint8_t)(((src2 & 0x3FU) << 2) | (src3 >> 8));
        *pDst++ = (uint8_t)(src3);
    }

    if (length & 3U) {
        uint32_t src0 = (uint32_t)pSrc[i+0];
        uint32_t src1 = 0U;
        uint32_t src2 = 0U;
        length = length & 3U;

        if (length > 1U) {
            src1 = (uint32_t)pSrc[i+1];
            if (length > 2U) {
                src2 = (uint32_t)pSrc[i+2];
            }
        }
        *pDst++ = (uint8_t)(src0 >> 2);
        *pDst++ = (uint8_t)(((src0 & 0x3U) << 6) | (src1 >> 4));
        if (length > 1U) {
            *pDst++ = (uint8_t)(((src1 & 0xFU) << 4) | (src2 >> 6));
            if (length > 2U) {
                *pDst++ = (uint8_t)(((src2 & 0x3FU) << 2));
            }
        }
    }
}

static void tif_32sto11u(const int32_t* pSrc, OPJ_BYTE* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;

	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t src0 = (uint32_t)pSrc[i + 0];
		uint32_t src1 = (uint32_t)pSrc[i + 1];
		uint32_t src2 = (uint32_t)pSrc[i + 2];
		uint32_t src3 = (uint32_t)pSrc[i + 3];
		uint32_t src4 = (uint32_t)pSrc[i + 4];
		uint32_t src5 = (uint32_t)pSrc[i + 5];
		uint32_t src6 = (uint32_t)pSrc[i + 6];
		uint32_t src7 = (uint32_t)pSrc[i + 7];

		*pDst++ = (OPJ_BYTE)((src0 >> 3));
		*pDst++ = (OPJ_BYTE)((src0 << 5) | (src1 >> 6));
		*pDst++ = (OPJ_BYTE)((src1 << 2) | (src2 >> 9));
		*pDst++ = (OPJ_BYTE)((src2 >> 1));
		*pDst++ = (OPJ_BYTE)((src2 << 7) | (src3 >> 4));
		*pDst++ = (OPJ_BYTE)((src3 << 4) | (src4 >> 7));
		*pDst++ = (OPJ_BYTE)((src4 << 1) | (src5 >> 10));
		*pDst++ = (OPJ_BYTE)((src5 >> 2));
		*pDst++ = (OPJ_BYTE)((src5 << 6) | (src6 >> 5));
		*pDst++ = (OPJ_BYTE)((src6 << 3) | (src7 >> 8));
		*pDst++ = (OPJ_BYTE)(src7);
	}

	if (length & 7U) {
		unsigned int trailing = 0U;
		int remaining = 8U;
		length &= 7U;
		PUTBITS2((uint32_t)pSrc[i + 0], 11)
			if (length > 1U) {
				PUTBITS2((uint32_t)pSrc[i + 1], 11)
					if (length > 2U) {
						PUTBITS2((uint32_t)pSrc[i + 2], 11)
							if (length > 3U) {
								PUTBITS2((uint32_t)pSrc[i + 3], 11)
									if (length > 4U) {
										PUTBITS2((uint32_t)pSrc[i + 4], 11)
											if (length > 5U) {
												PUTBITS2((uint32_t)pSrc[i + 5], 11)
													if (length > 6U) {
														PUTBITS2((uint32_t)pSrc[i + 6], 11)
													}
											}
									}
							}
					}
			}
		FLUSHBITS()
	}
}
static void tif_32sto12u(const int32_t* pSrc, uint8_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < (length & ~(size_t)1U); i+=2U) {
        uint32_t src0 = (uint32_t)pSrc[i+0];
        uint32_t src1 = (uint32_t)pSrc[i+1];

        *pDst++ = (uint8_t)(src0 >> 4);
        *pDst++ = (uint8_t)(((src0 & 0xFU) << 4) | (src1 >> 8));
        *pDst++ = (uint8_t)(src1);
    }

    if (length & 1U) {
        uint32_t src0 = (uint32_t)pSrc[i+0];
        *pDst++ = (uint8_t)(src0 >> 4);
        *pDst++ = (uint8_t)(((src0 & 0xFU) << 4));
    }
}

static void tif_32sto13u(const int32_t* pSrc, OPJ_BYTE* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;

	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t src0 = (uint32_t)pSrc[i + 0];
		uint32_t src1 = (uint32_t)pSrc[i + 1];
		uint32_t src2 = (uint32_t)pSrc[i + 2];
		uint32_t src3 = (uint32_t)pSrc[i + 3];
		uint32_t src4 = (uint32_t)pSrc[i + 4];
		uint32_t src5 = (uint32_t)pSrc[i + 5];
		uint32_t src6 = (uint32_t)pSrc[i + 6];
		uint32_t src7 = (uint32_t)pSrc[i + 7];

		*pDst++ = (OPJ_BYTE)((src0 >> 5));
		*pDst++ = (OPJ_BYTE)((src0 << 3) | (src1 >> 10));
		*pDst++ = (OPJ_BYTE)((src1 >> 2));
		*pDst++ = (OPJ_BYTE)((src1 << 6) | (src2 >> 7));
		*pDst++ = (OPJ_BYTE)((src2 << 1) | (src3 >> 12));
		*pDst++ = (OPJ_BYTE)((src3 >> 4));
		*pDst++ = (OPJ_BYTE)((src3 << 4) | (src4 >> 9));
		*pDst++ = (OPJ_BYTE)((src4 >> 1));
		*pDst++ = (OPJ_BYTE)((src4 << 7) | (src5 >> 6));
		*pDst++ = (OPJ_BYTE)((src5 << 2) | (src6 >> 11));
		*pDst++ = (OPJ_BYTE)((src6 >> 3));
		*pDst++ = (OPJ_BYTE)((src6 << 5) | (src7 >> 8));
		*pDst++ = (OPJ_BYTE)(src7);
	}

	if (length & 7U) {
		unsigned int trailing = 0U;
		int remaining = 8U;
		length &= 7U;
		PUTBITS2((uint32_t)pSrc[i + 0], 13)
			if (length > 1U) {
				PUTBITS2((uint32_t)pSrc[i + 1], 13)
					if (length > 2U) {
						PUTBITS2((uint32_t)pSrc[i + 2], 13)
							if (length > 3U) {
								PUTBITS2((uint32_t)pSrc[i + 3], 13)
									if (length > 4U) {
										PUTBITS2((uint32_t)pSrc[i + 4], 13)
											if (length > 5U) {
												PUTBITS2((uint32_t)pSrc[i + 5], 13)
													if (length > 6U) {
														PUTBITS2((uint32_t)pSrc[i + 6], 13)
													}
											}
									}
							}
					}
			}
		FLUSHBITS()
	}
}

static void tif_32sto14u(const int32_t* pSrc, uint8_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < (length & ~(size_t)3U); i+=4U) {
        uint32_t src0 = (uint32_t)pSrc[i+0];
        uint32_t src1 = (uint32_t)pSrc[i+1];
        uint32_t src2 = (uint32_t)pSrc[i+2];
        uint32_t src3 = (uint32_t)pSrc[i+3];

        *pDst++ = (uint8_t)(src0 >> 6);
        *pDst++ = (uint8_t)(((src0 & 0x3FU) << 2) | (src1 >> 12));
        *pDst++ = (uint8_t)(src1 >> 4);
        *pDst++ = (uint8_t)(((src1 & 0xFU) << 4) | (src2 >> 10));
        *pDst++ = (uint8_t)(src2 >> 2);
        *pDst++ = (uint8_t)(((src2 & 0x3U) << 6) | (src3 >> 8));
        *pDst++ = (uint8_t)(src3);
    }

    if (length & 3U) {
        uint32_t src0 = (uint32_t)pSrc[i+0];
        uint32_t src1 = 0U;
        uint32_t src2 = 0U;
        length = length & 3U;

        if (length > 1U) {
            src1 = (uint32_t)pSrc[i+1];
            if (length > 2U) {
                src2 = (uint32_t)pSrc[i+2];
            }
        }
        *pDst++ = (uint8_t)(src0 >> 6);
        *pDst++ = (uint8_t)(((src0 & 0x3FU) << 2) | (src1 >> 12));
        if (length > 1U) {
            *pDst++ = (uint8_t)(src1 >> 4);
            *pDst++ = (uint8_t)(((src1 & 0xFU) << 4) | (src2 >> 10));
            if (length > 2U) {
                *pDst++ = (uint8_t)(src2 >> 2);
                *pDst++ = (uint8_t)(((src2 & 0x3U) << 6));
            }
        }
    }
}

static void tif_32sto15u(const int32_t* pSrc, OPJ_BYTE* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;

	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t src0 = (uint32_t)pSrc[i + 0];
		uint32_t src1 = (uint32_t)pSrc[i + 1];
		uint32_t src2 = (uint32_t)pSrc[i + 2];
		uint32_t src3 = (uint32_t)pSrc[i + 3];
		uint32_t src4 = (uint32_t)pSrc[i + 4];
		uint32_t src5 = (uint32_t)pSrc[i + 5];
		uint32_t src6 = (uint32_t)pSrc[i + 6];
		uint32_t src7 = (uint32_t)pSrc[i + 7];

		*pDst++ = (OPJ_BYTE)((src0 >> 7));
		*pDst++ = (OPJ_BYTE)((src0 << 1) | (src1 >> 14));
		*pDst++ = (OPJ_BYTE)((src1 >> 6));
		*pDst++ = (OPJ_BYTE)((src1 << 2) | (src2 >> 13));
		*pDst++ = (OPJ_BYTE)((src2 >> 5));
		*pDst++ = (OPJ_BYTE)((src2 << 3) | (src3 >> 12));
		*pDst++ = (OPJ_BYTE)((src3 >> 4));
		*pDst++ = (OPJ_BYTE)((src3 << 4) | (src4 >> 11));
		*pDst++ = (OPJ_BYTE)((src4 >> 3));
		*pDst++ = (OPJ_BYTE)((src4 << 5) | (src5 >> 10));
		*pDst++ = (OPJ_BYTE)((src5 >> 2));
		*pDst++ = (OPJ_BYTE)((src5 << 6) | (src6 >> 9));
		*pDst++ = (OPJ_BYTE)((src6 >> 1));
		*pDst++ = (OPJ_BYTE)((src6 << 7) | (src7 >> 8));
		*pDst++ = (OPJ_BYTE)(src7);
	}

	if (length & 7U) {
		unsigned int trailing = 0U;
		int remaining = 8U;
		length &= 7U;
		PUTBITS2((uint32_t)pSrc[i + 0], 15)
			if (length > 1U) {
				PUTBITS2((uint32_t)pSrc[i + 1], 15)
					if (length > 2U) {
						PUTBITS2((uint32_t)pSrc[i + 2], 15)
							if (length > 3U) {
								PUTBITS2((uint32_t)pSrc[i + 3], 15)
									if (length > 4U) {
										PUTBITS2((uint32_t)pSrc[i + 4], 15)
											if (length > 5U) {
												PUTBITS2((uint32_t)pSrc[i + 5], 15)
													if (length > 6U) {
														PUTBITS2((uint32_t)pSrc[i + 6], 15)
													}
											}
									}
							}
					}
			}
		FLUSHBITS()
	}
}

static void tif_32sto16u(const int32_t* pSrc, uint16_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i) {
        pDst[i] = (uint16_t)pSrc[i];
    }
}

int imagetotif(opj_image_t * image, const char *outfile, uint32_t compression)
{
    uint32_t width, height;
	uint32_t bps,adjust, sgnd;
    int tiPhoto;
    TIFF *tif;
    tdata_t buf;
    tsize_t strip_size;
    uint32_t i, numcomps;
    size_t rowStride;
    int32_t* buffer32s = NULL;
    int32_t const* planes[4];
    convert_32s_PXCX cvtPxToCx = NULL;
    convert_32sXXx_C1R cvt32sToTif = NULL;

	// actual bits per sample
    bps = image->comps[0].prec;

	// even bits per sample
	uint32_t tif_bps = bps;

    planes[0] = image->comps[0].data;
    numcomps = image->numcomps;

    if (image->color_space == OPJ_CLRSPC_CMYK) {
        if (numcomps < 4U) {
            fprintf(stderr,"imagetotif: CMYK images shall be composed of at least 4 planes.\n");
            fprintf(stderr,"\tAborting\n");
            return 1;
        }
        tiPhoto = PHOTOMETRIC_SEPARATED;
        if (numcomps > 4U) {
            numcomps = 4U; /* Alpha not supported */
        }
    } else if (numcomps > 2U) {
        tiPhoto = PHOTOMETRIC_RGB;
        if (numcomps > 4U) {
            numcomps = 4U;
        }
    } else {
        tiPhoto = PHOTOMETRIC_MINISBLACK;
    }
    for (i = 1U; i < numcomps; ++i) {
        if (image->comps[0].dx != image->comps[i].dx) {
            break;
        }
        if (image->comps[0].dy != image->comps[i].dy) {
            break;
        }
        if (image->comps[0].prec != image->comps[i].prec) {
            break;
        }
        if (image->comps[0].sgnd != image->comps[i].sgnd) {
            break;
        }
        planes[i] = image->comps[i].data;
    }
    if (i != numcomps) {
        fprintf(stderr,"imagetotif: All components shall have the same subsampling, same bit depth.\n");
        fprintf(stderr,"\tAborting\n");
        return 1;
    }

	if (bps > 16) 
		bps = 0;
	if (bps == 0)
	{
		fprintf(stderr, "imagetotif: Bits=%d, Only 1 to 16 bits implemented\n", bps);
		fprintf(stderr, "\tAborting\n");
		return 1;
	}
    tif = TIFFOpen(outfile, "wb");
    if (!tif) {
        fprintf(stderr, "imagetotif:failed to open %s for writing\n", outfile);
        return 1;
    }
    for (i = 0U; i < numcomps; ++i) {
        clip_component(&(image->comps[i]), image->comps[0].prec);
    }
    cvtPxToCx = convert_32s_PXCX_LUT[numcomps];
    switch (tif_bps) {
    case 1:
    case 2:
    case 4:
    case 6:
    case 8:
        cvt32sToTif = convert_32sXXu_C1R_LUT[tif_bps];
        break;
	case 3:
		cvt32sToTif = tif_32sto3u;
		break;
	case 5:
		cvt32sToTif = tif_32sto5u;
		break;
	case 7:
		cvt32sToTif = tif_32sto7u;
		break;
	case 9:
		cvt32sToTif = tif_32sto9u;
		break;
    case 10:
        cvt32sToTif = tif_32sto10u;
        break;
	case 11:
		cvt32sToTif = tif_32sto11u;
		break;
    case 12:
        cvt32sToTif = tif_32sto12u;
        break;
	case 13:
		cvt32sToTif = tif_32sto13u;
		break;
    case 14:
        cvt32sToTif = tif_32sto14u;
        break;
	case 15:
		cvt32sToTif = tif_32sto15u;
		break;
    case 16:
        cvt32sToTif = (convert_32sXXx_C1R)tif_32sto16u;
        break;
    default:
        /* never here */
        break;
    }
    sgnd = (int)image->comps[0].sgnd;
    adjust = sgnd ? 1 << (image->comps[0].prec - 1) : 0;
    width   = (int)image->comps[0].w;
    height  = (int)image->comps[0].h;

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, numcomps);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, tif_bps);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, tiPhoto);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
	if (compression == COMPRESSION_ADOBE_DEFLATE) {
#ifdef ZIP_SUPPORT
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);  // zip compression
#endif
	}

	if (image->capture_resolution[0] > 0 && image->capture_resolution[1] > 0) {
		TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, 3); // cm
		for (int i = 0; i < 2; ++i) {
			TIFFSetField(tif, TIFFTAG_XRESOLUTION, image->capture_resolution[0]/100);
			TIFFSetField(tif, TIFFTAG_YRESOLUTION, image->capture_resolution[1]/100);
		}
	}

    strip_size = TIFFStripSize(tif);
    rowStride = ((size_t)width * numcomps * (size_t)tif_bps + 7U) / 8U;
    if (rowStride != (size_t)strip_size) {
        fprintf(stderr, "Invalid TIFF strip size\n");
        TIFFClose(tif);
        return 1;
    }
    buf = _TIFFmalloc(strip_size);
    if (buf == NULL) {
        TIFFClose(tif);
        return 1;
    }
    buffer32s = (int32_t *)malloc((size_t)width * numcomps * sizeof(int32_t));
    if (buffer32s == NULL) {
        _TIFFfree(buf);
        TIFFClose(tif);
        return 1;
    }

    for (i = 0; i < image->comps[0].h; ++i) {
        cvtPxToCx(planes, buffer32s, (size_t)width, adjust);
        cvt32sToTif(buffer32s, (uint8_t *)buf, (size_t)width * numcomps);
        (void)TIFFWriteEncodedStrip(tif, i, (void*)buf, strip_size);
        planes[0] += width;
        planes[1] += width;
        planes[2] += width;
        planes[3] += width;
    }
    _TIFFfree((void*)buf);
    TIFFClose(tif);
    free(buffer32s);

    return 0;
}/* imagetotif() */


#define GETBITS(dest, nb) { \
	int needed = (nb); \
	unsigned int dst = 0U; \
	if (available == 0) { \
		val = *pSrc++; \
		available = 8; \
	} \
	while (needed > available) { \
		dst |= val & ((1U << available) - 1U); \
		needed -= available; \
		dst <<= needed; \
		val = *pSrc++; \
		available = 8; \
	} \
	dst |= (val >> (available - needed)) & ((1U << needed) - 1U); \
	available -= needed; \
	dest = (int32_t)dst; \
}

static void tif_3uto32s(const OPJ_BYTE* pSrc, int32_t* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;
	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t val0 = *pSrc++;
		uint32_t val1 = *pSrc++;
		uint32_t val2 = *pSrc++;

		pDst[i + 0] = (int32_t)((val0 >> 5));
		pDst[i + 1] = (int32_t)(((val0 & 0x1FU) >> 2));
		pDst[i + 2] = (int32_t)(((val0 & 0x3U) << 1) | (val1 >> 7));
		pDst[i + 3] = (int32_t)(((val1 & 0x7FU) >> 4));
		pDst[i + 4] = (int32_t)(((val1 & 0xFU) >> 1));
		pDst[i + 5] = (int32_t)(((val1 & 0x1U) << 2) | (val2 >> 6));
		pDst[i + 6] = (int32_t)(((val2 & 0x3FU) >> 3));
		pDst[i + 7] = (int32_t)(((val2 & 0x7U)));

	}
	if (length & 7U) {
		unsigned int val;
		int available = 0;

		length = length & 7U;

		GETBITS(pDst[i + 0], 3)

			if (length > 1U) {
				GETBITS(pDst[i + 1], 3)
					if (length > 2U) {
						GETBITS(pDst[i + 2], 3)
							if (length > 3U) {
								GETBITS(pDst[i + 3], 3)
									if (length > 4U) {
										GETBITS(pDst[i + 4], 3)
											if (length > 5U) {
												GETBITS(pDst[i + 5], 3)
													if (length > 6U) {
														GETBITS(pDst[i + 6], 3)
													}
											}
									}
							}
					}
			}
	}
}
static void tif_5uto32s(const OPJ_BYTE* pSrc, int32_t* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;
	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t val0 = *pSrc++;
		uint32_t val1 = *pSrc++;
		uint32_t val2 = *pSrc++;
		uint32_t val3 = *pSrc++;
		uint32_t val4 = *pSrc++;

		pDst[i + 0] = (int32_t)((val0 >> 3));
		pDst[i + 1] = (int32_t)(((val0 & 0x7U) << 2) | (val1 >> 6));
		pDst[i + 2] = (int32_t)(((val1 & 0x3FU) >> 1));
		pDst[i + 3] = (int32_t)(((val1 & 0x1U) << 4) | (val2 >> 4));
		pDst[i + 4] = (int32_t)(((val2 & 0xFU) << 1) | (val3 >> 7));
		pDst[i + 5] = (int32_t)(((val3 & 0x7FU) >> 2));
		pDst[i + 6] = (int32_t)(((val3 & 0x3U) << 3) | (val4 >> 5));
		pDst[i + 7] = (int32_t)(((val4 & 0x1FU)));

	}
	if (length & 7U) {
		unsigned int val;
		int available = 0;

		length = length & 7U;

		GETBITS(pDst[i + 0], 5)

			if (length > 1U) {
				GETBITS(pDst[i + 1], 5)
					if (length > 2U) {
						GETBITS(pDst[i + 2], 5)
							if (length > 3U) {
								GETBITS(pDst[i + 3], 5)
									if (length > 4U) {
										GETBITS(pDst[i + 4], 5)
											if (length > 5U) {
												GETBITS(pDst[i + 5], 5)
													if (length > 6U) {
														GETBITS(pDst[i + 6], 5)
													}
											}
									}
							}
					}
			}
	}
}
static void tif_7uto32s(const OPJ_BYTE* pSrc, int32_t* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;
	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t val0 = *pSrc++;
		uint32_t val1 = *pSrc++;
		uint32_t val2 = *pSrc++;
		uint32_t val3 = *pSrc++;
		uint32_t val4 = *pSrc++;
		uint32_t val5 = *pSrc++;
		uint32_t val6 = *pSrc++;

		pDst[i + 0] = (int32_t)((val0 >> 1));
		pDst[i + 1] = (int32_t)(((val0 & 0x1U) << 6) | (val1 >> 2));
		pDst[i + 2] = (int32_t)(((val1 & 0x3U) << 5) | (val2 >> 3));
		pDst[i + 3] = (int32_t)(((val2 & 0x7U) << 4) | (val3 >> 4));
		pDst[i + 4] = (int32_t)(((val3 & 0xFU) << 3) | (val4 >> 5));
		pDst[i + 5] = (int32_t)(((val4 & 0x1FU) << 2) | (val5 >> 6));
		pDst[i + 6] = (int32_t)(((val5 & 0x3FU) << 1) | (val6 >> 7));
		pDst[i + 7] = (int32_t)(((val6 & 0x7FU)));

	}
	if (length & 7U) {
		unsigned int val;
		int available = 0;

		length = length & 7U;

		GETBITS(pDst[i + 0], 7)

			if (length > 1U) {
				GETBITS(pDst[i + 1], 7)
					if (length > 2U) {
						GETBITS(pDst[i + 2], 7)
							if (length > 3U) {
								GETBITS(pDst[i + 3], 7)
									if (length > 4U) {
										GETBITS(pDst[i + 4], 7)
											if (length > 5U) {
												GETBITS(pDst[i + 5], 7)
													if (length > 6U) {
														GETBITS(pDst[i + 6], 7)
													}
											}
									}
							}
					}
			}
	}
}
static void tif_9uto32s(const OPJ_BYTE* pSrc, int32_t* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;
	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t val0 = *pSrc++;
		uint32_t val1 = *pSrc++;
		uint32_t val2 = *pSrc++;
		uint32_t val3 = *pSrc++;
		uint32_t val4 = *pSrc++;
		uint32_t val5 = *pSrc++;
		uint32_t val6 = *pSrc++;
		uint32_t val7 = *pSrc++;
		uint32_t val8 = *pSrc++;

		pDst[i + 0] = (int32_t)((val0 << 1) | (val1 >> 7));
		pDst[i + 1] = (int32_t)(((val1 & 0x7FU) << 2) | (val2 >> 6));
		pDst[i + 2] = (int32_t)(((val2 & 0x3FU) << 3) | (val3 >> 5));
		pDst[i + 3] = (int32_t)(((val3 & 0x1FU) << 4) | (val4 >> 4));
		pDst[i + 4] = (int32_t)(((val4 & 0xFU) << 5) | (val5 >> 3));
		pDst[i + 5] = (int32_t)(((val5 & 0x7U) << 6) | (val6 >> 2));
		pDst[i + 6] = (int32_t)(((val6 & 0x3U) << 7) | (val7 >> 1));
		pDst[i + 7] = (int32_t)(((val7 & 0x1U) << 8) | (val8));

	}
	if (length & 7U) {
		unsigned int val;
		int available = 0;

		length = length & 7U;

		GETBITS(pDst[i + 0], 9)

			if (length > 1U) {
				GETBITS(pDst[i + 1], 9)
					if (length > 2U) {
						GETBITS(pDst[i + 2], 9)
							if (length > 3U) {
								GETBITS(pDst[i + 3], 9)
									if (length > 4U) {
										GETBITS(pDst[i + 4], 9)
											if (length > 5U) {
												GETBITS(pDst[i + 5], 9)
													if (length > 6U) {
														GETBITS(pDst[i + 6], 9)
													}
											}
									}
							}
					}
			}
	}
}



static void tif_10uto32s(const uint8_t* pSrc, int32_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < (length & ~(size_t)3U); i+=4U) {
        uint32_t val0 = *pSrc++;
        uint32_t val1 = *pSrc++;
        uint32_t val2 = *pSrc++;
        uint32_t val3 = *pSrc++;
        uint32_t val4 = *pSrc++;

        pDst[i+0] = (int32_t)((val0 << 2) | (val1 >> 6));
        pDst[i+1] = (int32_t)(((val1 & 0x3FU) << 4) | (val2 >> 4));
        pDst[i+2] = (int32_t)(((val2 & 0xFU) << 6) | (val3 >> 2));
        pDst[i+3] = (int32_t)(((val3 & 0x3U) << 8) | val4);

    }
    if (length & 3U) {
        uint32_t val0 = *pSrc++;
        uint32_t val1 = *pSrc++;
        length = length & 3U;
        pDst[i+0] = (int32_t)((val0 << 2) | (val1 >> 6));

        if (length > 1U) {
            uint32_t val2 = *pSrc++;
            pDst[i+1] = (int32_t)(((val1 & 0x3FU) << 4) | (val2 >> 4));
            if (length > 2U) {
                uint32_t val3 = *pSrc++;
                pDst[i+2] = (int32_t)(((val2 & 0xFU) << 6) | (val3 >> 2));
            }
        }
    }
}

static void tif_11uto32s(const OPJ_BYTE* pSrc, int32_t* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;
	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t val0 = *pSrc++;
		uint32_t val1 = *pSrc++;
		uint32_t val2 = *pSrc++;
		uint32_t val3 = *pSrc++;
		uint32_t val4 = *pSrc++;
		uint32_t val5 = *pSrc++;
		uint32_t val6 = *pSrc++;
		uint32_t val7 = *pSrc++;
		uint32_t val8 = *pSrc++;
		uint32_t val9 = *pSrc++;
		uint32_t val10 = *pSrc++;

		pDst[i + 0] = (int32_t)((val0 << 3) | (val1 >> 5));
		pDst[i + 1] = (int32_t)(((val1 & 0x1FU) << 6) | (val2 >> 2));
		pDst[i + 2] = (int32_t)(((val2 & 0x3U) << 9) | (val3 << 1) | (val4 >> 7));
		pDst[i + 3] = (int32_t)(((val4 & 0x7FU) << 4) | (val5 >> 4));
		pDst[i + 4] = (int32_t)(((val5 & 0xFU) << 7) | (val6 >> 1));
		pDst[i + 5] = (int32_t)(((val6 & 0x1U) << 10) | (val7 << 2) | (val8 >> 6));
		pDst[i + 6] = (int32_t)(((val8 & 0x3FU) << 5) | (val9 >> 3));
		pDst[i + 7] = (int32_t)(((val9 & 0x7U) << 8) | (val10));

	}
	if (length & 7U) {
		unsigned int val;
		int available = 0;

		length = length & 7U;

		GETBITS(pDst[i + 0], 11)

			if (length > 1U) {
				GETBITS(pDst[i + 1], 11)
					if (length > 2U) {
						GETBITS(pDst[i + 2], 11)
							if (length > 3U) {
								GETBITS(pDst[i + 3], 11)
									if (length > 4U) {
										GETBITS(pDst[i + 4], 11)
											if (length > 5U) {
												GETBITS(pDst[i + 5], 11)
													if (length > 6U) {
														GETBITS(pDst[i + 6], 11)
													}
											}
									}
							}
					}
			}
	}
}
static void tif_12uto32s(const uint8_t* pSrc, int32_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < (length & ~(size_t)1U); i+=2U) {
        uint32_t val0 = *pSrc++;
        uint32_t val1 = *pSrc++;
        uint32_t val2 = *pSrc++;

        pDst[i+0] = (int32_t)((val0 << 4) | (val1 >> 4));
        pDst[i+1] = (int32_t)(((val1 & 0xFU) << 8) | val2);
    }
    if (length & 1U) {
        uint32_t val0 = *pSrc++;
        uint32_t val1 = *pSrc++;
        pDst[i+0] = (int32_t)((val0 << 4) | (val1 >> 4));
    }
}

static void tif_13uto32s(const OPJ_BYTE* pSrc, int32_t* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;
	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t val0 = *pSrc++;
		uint32_t val1 = *pSrc++;
		uint32_t val2 = *pSrc++;
		uint32_t val3 = *pSrc++;
		uint32_t val4 = *pSrc++;
		uint32_t val5 = *pSrc++;
		uint32_t val6 = *pSrc++;
		uint32_t val7 = *pSrc++;
		uint32_t val8 = *pSrc++;
		uint32_t val9 = *pSrc++;
		uint32_t val10 = *pSrc++;
		uint32_t val11 = *pSrc++;
		uint32_t val12 = *pSrc++;

		pDst[i + 0] = (int32_t)((val0 << 5) | (val1 >> 3));
		pDst[i + 1] = (int32_t)(((val1 & 0x7U) << 10) | (val2 << 2) | (val3 >> 6));
		pDst[i + 2] = (int32_t)(((val3 & 0x3FU) << 7) | (val4 >> 1));
		pDst[i + 3] = (int32_t)(((val4 & 0x1U) << 12) | (val5 << 4) | (val6 >> 4));
		pDst[i + 4] = (int32_t)(((val6 & 0xFU) << 9) | (val7 << 1) | (val8 >> 7));
		pDst[i + 5] = (int32_t)(((val8 & 0x7FU) << 6) | (val9 >> 2));
		pDst[i + 6] = (int32_t)(((val9 & 0x3U) << 11) | (val10 << 3) | (val11 >> 5));
		pDst[i + 7] = (int32_t)(((val11 & 0x1FU) << 8) | (val12));

	}
	if (length & 7U) {
		unsigned int val;
		int available = 0;

		length = length & 7U;

		GETBITS(pDst[i + 0], 13)

			if (length > 1U) {
				GETBITS(pDst[i + 1], 13)
					if (length > 2U) {
						GETBITS(pDst[i + 2], 13)
							if (length > 3U) {
								GETBITS(pDst[i + 3], 13)
									if (length > 4U) {
										GETBITS(pDst[i + 4], 13)
											if (length > 5U) {
												GETBITS(pDst[i + 5], 13)
													if (length > 6U) {
														GETBITS(pDst[i + 6], 13)
													}
											}
									}
							}
					}
			}
	}
}

static void tif_14uto32s(const uint8_t* pSrc, int32_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < (length & ~(size_t)3U); i+=4U) {
        uint32_t val0 = *pSrc++;
        uint32_t val1 = *pSrc++;
        uint32_t val2 = *pSrc++;
        uint32_t val3 = *pSrc++;
        uint32_t val4 = *pSrc++;
        uint32_t val5 = *pSrc++;
        uint32_t val6 = *pSrc++;

        pDst[i+0] = (int32_t)((val0 << 6) | (val1 >> 2));
        pDst[i+1] = (int32_t)(((val1 & 0x3U) << 12) | (val2 << 4) | (val3 >> 4));
        pDst[i+2] = (int32_t)(((val3 & 0xFU) << 10) | (val4 << 2) | (val5 >> 6));
        pDst[i+3] = (int32_t)(((val5 & 0x3FU) << 8) | val6);

    }
    if (length & 3U) {
        uint32_t val0 = *pSrc++;
        uint32_t val1 = *pSrc++;
        length = length & 3U;
        pDst[i+0] = (int32_t)((val0 << 6) | (val1 >> 2));

        if (length > 1U) {
            uint32_t val2 = *pSrc++;
            uint32_t val3 = *pSrc++;
            pDst[i+1] = (int32_t)(((val1 & 0x3U) << 12) | (val2 << 4) | (val3 >> 4));
            if (length > 2U) {
                uint32_t val4 = *pSrc++;
                uint32_t val5 = *pSrc++;
                pDst[i+2] = (int32_t)(((val3 & 0xFU) << 10) | (val4 << 2) | (val5 >> 6));
            }
        }
    }
}

static void tif_15uto32s(const OPJ_BYTE* pSrc, int32_t* pDst, OPJ_SIZE_T length)
{
	OPJ_SIZE_T i;
	for (i = 0; i < (length & ~(OPJ_SIZE_T)7U); i += 8U) {
		uint32_t val0 = *pSrc++;
		uint32_t val1 = *pSrc++;
		uint32_t val2 = *pSrc++;
		uint32_t val3 = *pSrc++;
		uint32_t val4 = *pSrc++;
		uint32_t val5 = *pSrc++;
		uint32_t val6 = *pSrc++;
		uint32_t val7 = *pSrc++;
		uint32_t val8 = *pSrc++;
		uint32_t val9 = *pSrc++;
		uint32_t val10 = *pSrc++;
		uint32_t val11 = *pSrc++;
		uint32_t val12 = *pSrc++;
		uint32_t val13 = *pSrc++;
		uint32_t val14 = *pSrc++;

		pDst[i + 0] = (int32_t)((val0 << 7) | (val1 >> 1));
		pDst[i + 1] = (int32_t)(((val1 & 0x1U) << 14) | (val2 << 6) | (val3 >> 2));
		pDst[i + 2] = (int32_t)(((val3 & 0x3U) << 13) | (val4 << 5) | (val5 >> 3));
		pDst[i + 3] = (int32_t)(((val5 & 0x7U) << 12) | (val6 << 4) | (val7 >> 4));
		pDst[i + 4] = (int32_t)(((val7 & 0xFU) << 11) | (val8 << 3) | (val9 >> 5));
		pDst[i + 5] = (int32_t)(((val9 & 0x1FU) << 10) | (val10 << 2) | (val11 >> 6));
		pDst[i + 6] = (int32_t)(((val11 & 0x3FU) << 9) | (val12 << 1) | (val13 >> 7));
		pDst[i + 7] = (int32_t)(((val13 & 0x7FU) << 8) | (val14));

	}
	if (length & 7U) {
		unsigned int val;
		int available = 0;

		length = length & 7U;

		GETBITS(pDst[i + 0], 15)

			if (length > 1U) {
				GETBITS(pDst[i + 1], 15)
					if (length > 2U) {
						GETBITS(pDst[i + 2], 15)
							if (length > 3U) {
								GETBITS(pDst[i + 3], 15)
									if (length > 4U) {
										GETBITS(pDst[i + 4], 15)
											if (length > 5U) {
												GETBITS(pDst[i + 5], 15)
													if (length > 6U) {
														GETBITS(pDst[i + 6], 15)
													}
											}
									}
							}
					}
			}
	}
}

/* seems that libtiff decodes this to machine endianness */
static void tif_16uto32s(const uint16_t* pSrc, int32_t* pDst, size_t length)
{
    size_t i;
    for (i = 0; i < length; i++) {
        pDst[i] = pSrc[i];
    }
}

static void set_resolution(double* res, float resx, float resy, short resUnit) {
	// resolution is in pels / metre
	res[0] = resx;
	res[1] = resy;

	switch (resUnit) {
		// no known units. Used for images with non-square pixels
	case 1:
		break;
		// inches  
	case 2:
		//2.54 cm / inch
		res[0] *= 100 / 2.54;
		res[1] *= 100 / 2.54;
		break;
		// cm
	case 3:
		res[0] *= 100;
		res[1] *= 100;
		break;
	default:
		res[0] = 0;
		res[1] = 0;
		break;
	}
}

/*
 * libtiff/tif_getimage.c : 1,2,4,8,16 bitspersample accepted
 * CINEMA                 : 12 bit precision
 */
opj_image_t* tiftoimage(const char *filename, opj_cparameters_t *parameters)
{
    uint32_t subsampling_dx = parameters->subsampling_dx;
	uint32_t subsampling_dy = parameters->subsampling_dy;
    TIFF *tif;
    tdata_t buf = NULL;
    tstrip_t strip;
    tsize_t strip_size;
	uint32_t j, currentPlane, numcomps = 0, w, h;
    OPJ_COLOR_SPACE color_space = OPJ_CLRSPC_UNKNOWN;
    opj_image_cmptparm_t cmptparm[4]; /* RGBA */
    opj_image_t *image = NULL;
    int has_alpha = 0;
	unsigned short tiBps=0, tiPhoto=0, tiSf=0, tiSpp=0, tiPC=0;
	short tiResUnit=0;
	float tiXRes=0, tiYRes=0;
    unsigned int tiWidth=0, tiHeight=0;
    bool is_cinema = OPJ_IS_CINEMA(parameters->rsiz);
    convert_XXx32s_C1R cvtTifTo32s = NULL;
    convert_32s_CXPX cvtCxToPx = NULL;
    int32_t* buffer32s = NULL;
    int32_t* planes[4];
    size_t rowStride;
	bool success = true;

    tif = TIFFOpen(filename, "r");

    if(!tif) {
        fprintf(stderr, "tiftoimage:Failed to open %s for reading\n", filename);
        return 0;
    }
    tiBps = tiPhoto = tiSf = tiSpp = tiPC = 0;
    tiWidth = tiHeight = 0;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &tiWidth);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &tiHeight);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &tiBps);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &tiSf);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &tiSpp);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &tiPhoto);
    TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &tiPC);
	TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &tiPC);

	// if write_capture_resolution is enabled but capture_resolution equals 0,0, then
	// use image resolution if present
	if (parameters->write_capture_resolution &&
		parameters->capture_resolution[0] == 0 &&
			parameters->capture_resolution[1] == 0) {

		TIFFGetField(tif, TIFFTAG_XRESOLUTION, &tiXRes);
		TIFFGetField(tif, TIFFTAG_YRESOLUTION, &tiYRes);
		TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &tiResUnit);
		set_resolution(parameters->capture_resolution, tiXRes, tiYRes, tiResUnit);
	}

	// if write_display_resolution is enabled but display_resolution equals 0,0, then
	// use image resolution if present
	if (parameters->write_display_resolution &&
		parameters->display_resolution[0] == 0 &&
		parameters->display_resolution[1] == 0) {

		TIFFGetField(tif, TIFFTAG_XRESOLUTION, &tiXRes);
		TIFFGetField(tif, TIFFTAG_YRESOLUTION, &tiYRes);
		TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &tiResUnit);
		set_resolution(parameters->display_resolution, tiXRes, tiYRes, tiResUnit);
	}

    w= (int)tiWidth;
    h= (int)tiHeight;

	if (tiBps > 16U) {
		fprintf(stderr, "tiftoimage: Bits=%d, Only 1 to 16 bits implemented\n", tiBps);
		fprintf(stderr, "\tAborting\n");
		TIFFClose(tif);
		return NULL;
	}

    if(tiPhoto != PHOTOMETRIC_MINISBLACK && tiPhoto != PHOTOMETRIC_RGB) {
        fprintf(stderr,"tiftoimage: Bad color format %d.\n\tOnly RGB(A) and GRAY(A) has been implemented\n",(int) tiPhoto);
        fprintf(stderr,"\tAborting\n");
        TIFFClose(tif);
        return NULL;
    }

    switch (tiBps) {
    case 1:
    case 2:
    case 4:
    case 6:
    case 8:
        cvtTifTo32s = convert_XXu32s_C1R_LUT[tiBps];
        break;
    /* others are specific to TIFF */
	case 3:
		cvtTifTo32s = tif_3uto32s;
		break;
	case 5:
		cvtTifTo32s = tif_5uto32s;
		break;
	case 7:
		cvtTifTo32s = tif_7uto32s;
		break;
	case 9:
		cvtTifTo32s = tif_9uto32s;
		break;
    case 10:
        cvtTifTo32s = tif_10uto32s;
        break;
	case 11:
		cvtTifTo32s = tif_11uto32s;
		break;
    case 12:
        cvtTifTo32s = tif_12uto32s;
        break;
	case 13:
		cvtTifTo32s = tif_13uto32s;
		break;
    case 14:
        cvtTifTo32s = tif_14uto32s;
        break;
	case 15:
		cvtTifTo32s = tif_15uto32s;
		break;
    case 16:
        cvtTifTo32s = (convert_XXx32s_C1R)tif_16uto32s;
        break;
    default:
        /* never here */
        break;
    }

    {/* From: tiff-4.0.x/libtiff/tif_getimage.c : */
        uint16* sampleinfo;
        uint16 extrasamples;

        TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,
                              &extrasamples, &sampleinfo);

        if(extrasamples >= 1) {
            switch(sampleinfo[0]) {
            case EXTRASAMPLE_UNSPECIFIED:
                /* Workaround for some images without correct info about alpha channel
                 */
                if(tiSpp > 3)
                    has_alpha = 1;
                break;

            case EXTRASAMPLE_ASSOCALPHA: /* data pre-multiplied */
            case EXTRASAMPLE_UNASSALPHA: /* data not pre-multiplied */
                has_alpha = 1;
                break;
            }
        } else /* extrasamples == 0 */
            if(tiSpp == 4 || tiSpp == 2) has_alpha = 1;
    }

    /* initialize image components */
    memset(&cmptparm[0], 0, 4 * sizeof(opj_image_cmptparm_t));

    if ((tiPhoto == PHOTOMETRIC_RGB) && (is_cinema) && (tiBps != 12U)) {
        fprintf(stdout,"WARNING:\n"
                "Input image bitdepth is %d bits\n"
                "TIF conversion has automatically rescaled to 12-bits\n"
                "to comply with cinema profiles.\n",
                tiBps);
    } else {
        is_cinema = 0U;
    }

    if(tiPhoto == PHOTOMETRIC_RGB) { /* RGB(A) */
        numcomps = 3 + has_alpha;
        color_space = OPJ_CLRSPC_SRGB;
    } else if (tiPhoto == PHOTOMETRIC_MINISBLACK) { /* GRAY(A) */
        numcomps = 1 + has_alpha;
        color_space = OPJ_CLRSPC_GRAY;
    }

    cvtCxToPx = convert_32s_CXPX_LUT[numcomps];
    if (tiPC == PLANARCONFIG_SEPARATE) {
        cvtCxToPx = convert_32s_CXPX_LUT[1]; /* override */
        tiSpp = 1U; /* consider only one sample per plane */
    }

    for(j = 0; j < numcomps; j++) {
        cmptparm[j].prec = tiBps;
        cmptparm[j].dx = subsampling_dx;
        cmptparm[j].dy = subsampling_dy;
        cmptparm[j].w = w;
        cmptparm[j].h = h;
    }
	uint32_t icclen = 0;
	uint8_t* iccbuf = NULL;
    image = opj_image_create(numcomps, &cmptparm[0], color_space);
    if(!image) {
		success = false;
		goto cleanup;
    }
    /* set image offset and reference grid */
    image->x0 = parameters->image_offset_x0;
    image->y0 = parameters->image_offset_y0;
    image->x1 =	!image->x0 ? (w - 1) * subsampling_dx + 1 :
                image->x0 + (w - 1) * subsampling_dx + 1;
    image->y1 =	!image->y0 ? (h - 1) * subsampling_dy + 1 :
                image->y0 + (h - 1) * subsampling_dy + 1;

    for(j = 0; j < numcomps; j++) {
        planes[j] = image->comps[j].data;
    }
    image->comps[numcomps - 1].alpha = (1 - (numcomps & 1));

	// handle embedded ICC profile
	if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &icclen, &iccbuf) &&
		icclen > 0 &&
		icclen < 1000000000) {
		image->icc_profile_len = icclen;
		image->icc_profile_buf = (uint8_t*)malloc(icclen);
		if (!image->icc_profile_buf) {
			success = false;
			goto cleanup;
		}
		memcpy(image->icc_profile_buf, iccbuf, icclen);
	}


    strip_size = TIFFStripSize(tif);
    buf = _TIFFmalloc(strip_size);
    if (buf == NULL) {
		success = false;
		goto cleanup;
    }
    rowStride = ((size_t)w * tiSpp * tiBps + 7U) / 8U;
    buffer32s = (int32_t *)malloc((size_t)w * tiSpp * sizeof(int32_t));
    if (buffer32s == NULL) {
		success = false;
		goto cleanup;
    }

    strip = 0;
    currentPlane = 0;
    do {
        planes[0] = image->comps[currentPlane].data; /* to manage planar data */
        h= (int)tiHeight;
        /* Read the Image components */
        for(; (h > 0) && (strip < TIFFNumberOfStrips(tif)); strip++) {
            const uint8_t *dat8;
            size_t ssize=0;

            ssize = (size_t)TIFFReadEncodedStrip(tif, strip, buf, strip_size);
			if (ssize == (size_t)-1) {
				success = false;
				goto cleanup;
			}
            dat8 = (const uint8_t*)buf;

            while (ssize >= rowStride) {
                cvtTifTo32s(dat8, buffer32s, (size_t)w * tiSpp);
                cvtCxToPx(buffer32s, planes, (size_t)w);
                planes[0] += w;
                planes[1] += w;
                planes[2] += w;
                planes[3] += w;
                dat8  += rowStride;
                ssize -= rowStride;
                h--;
            }
        }
        currentPlane++;
    } while ((tiPC == PLANARCONFIG_SEPARATE) && (currentPlane < numcomps));

cleanup:
	if (buffer32s)
		free(buffer32s);
	if (buf)
		_TIFFfree(buf);
	if (tif)
		TIFFClose(tif);

	if (success) {
		if (is_cinema) {
			for (j = 0; j < numcomps; ++j) {
				scale_component(&(image->comps[j]), 12);
			}
		}
		return image;
	}

	if (image)
		opj_image_destroy(image);
	return NULL;

}/* tiftoimage() */


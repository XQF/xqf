/*
 * huffman.c
 * huffman encoding/decoding for use in hexenworld networking
 *
 * Copyright (C) 1997-1998  Raven Software Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "huffman.h"

//
// huffman types and vars
//

typedef struct huffnode_s
{
	struct huffnode_s *zero;
	struct huffnode_s *one;
	float		freq;
	unsigned char	val;
	unsigned char	pad[3];
} huffnode_t;

typedef struct
{
	unsigned int	bits;
	int		len;
} hufftab_t;

static void *HuffMemBase = NULL;
static huffnode_t *HuffTree = NULL;
static hufftab_t HuffLookup[256];

static const float HuffFreq[256] =
{
#	include "hufffreq.h"
};

int	huff_failed = 0;


//=============================================================================

//
// huffman functions
//

static void FindTab (huffnode_t *tmp, int len, unsigned int bits)
{
	if (huff_failed)
		return;
	if (!tmp)
	{
		huff_failed = 1;
		return;
	}

	if (tmp->zero)
	{
		if (!tmp->one)
		{
			huff_failed = 1;
			return;
		}
		if (len >= 32)
		{
			huff_failed = 1;
			return;
		}
		FindTab (tmp->zero, len+1, bits<<1);
		FindTab (tmp->one, len+1, (bits<<1)|1);
		return;
	}

	HuffLookup[tmp->val].len = len;
	HuffLookup[tmp->val].bits = bits;
	return;
}

static unsigned char const Masks[8] =
{
	0x1,
	0x2,
	0x4,
	0x8,
	0x10,
	0x20,
	0x40,
	0x80
};

static void PutBit (unsigned char *buf, int pos, int bit)
{
	if (bit)
		buf[pos/8] |= Masks[pos%8];
	else
		buf[pos/8] &=~Masks[pos%8];
}

static int GetBit (const unsigned char *buf, int pos)
{
	if (buf[pos/8] & Masks[pos%8])
		return 1;
	else
		return 0;
}

static void BuildTree (const float *freq)
{
	float	min1, min2;
	int	i, j, minat1, minat2;
	huffnode_t	*work[256];
	huffnode_t	*tmp;

	HuffMemBase = malloc(512 * sizeof(huffnode_t));
	if (!HuffMemBase)
	{
		huff_failed = 1;
		return;
	}
	memset(HuffMemBase, 0, 512 * sizeof(huffnode_t));
	tmp = (huffnode_t *) HuffMemBase;

	for (i = 0; i < 256; tmp++, i++)
	{
		tmp->val = (unsigned char)i;
		tmp->freq = freq[i];
		tmp->zero = NULL;
		tmp->one = NULL;
		HuffLookup[i].len = 0;
		work[i] = tmp;
	}

	for (i = 0; i < 255; tmp++, i++)
	{
		minat1 = -1;
		minat2 = -1;
		min1 = 1E30;
		min2 = 1E30;

		for (j = 0; j < 256; j++)
		{
			if (!work[j])
				continue;
			if (work[j]->freq < min1)
			{
				minat2 = minat1;
				min2 = min1;
				minat1 = j;
				min1 = work[j]->freq;
			}
			else if (work[j]->freq < min2)
			{
				minat2 = j;
				min2 = work[j]->freq;
			}
		}
		if (minat1 < 0)
		{
			huff_failed = 1;
			goto finish;
		}
		if (minat2 < 0)
		{
			huff_failed = 1;
			goto finish;
		}
		tmp->zero = work[minat2];
		tmp->one = work[minat1];
		tmp->freq = work[minat2]->freq + work[minat1]->freq;
		tmp->val = 0xff;
		work[minat1] = tmp;
		work[minat2] = NULL;
	}

	HuffTree = --tmp; // last incrementation in the loop above wasn't used
	FindTab (HuffTree, 0, 0);
finish:
	if (huff_failed)
		free (HuffMemBase);
}

void HuffDecode (const unsigned char *in, unsigned char *out, ssize_t inlen, ssize_t *outlen, const int maxlen)
{
	int	bits, tbits;
	huffnode_t	*tmp;

	--inlen;
	if (inlen < 0)
	{
		*outlen = 0;
		return;
	}
	if (*in == 0xff)
	{
		if (inlen > maxlen)
			memcpy (out, in+1, maxlen);
		else if (inlen)
			memcpy (out, in+1, inlen);
		*outlen = inlen;
		return;
	}

	tbits = inlen*8 - *in;
	bits = 0;
	*outlen = 0;

	while (bits < tbits)
	{
		tmp = HuffTree;
		do
		{
			if ( GetBit(in+1, bits) )
				tmp = tmp->one;
			else
				tmp = tmp->zero;
			bits++;
		} while (tmp->zero);

		if ( ++(*outlen) > maxlen )
			return;	// out[maxlen - 1] is written already
		*out++ = tmp->val;
	}
}

void HuffEncode (const unsigned char *in, unsigned char *out, ssize_t inlen, ssize_t *outlen)
{
	int	i, j, bitat;
	unsigned int	t;

	bitat = 0;

	for (i = 0; i < inlen; i++)
	{
		t = HuffLookup[in[i]].bits;
		for (j = 0; j < HuffLookup[in[i]].len; j++)
		{
			PutBit (out+1, bitat + HuffLookup[in[i]].len-j-1, t&1);
			t >>= 1;
		}
		bitat += HuffLookup[in[i]].len;
	}

	*outlen = 1 + (bitat + 7)/8;
	*out = 8 * ((*outlen)-1) - bitat;

	if (*outlen >= inlen+1)
	{
		*out = 0xff;
		memcpy (out+1, in, inlen);
		*outlen = inlen+1;
	}
}

void HuffInit (void)
{
	BuildTree(HuffFreq);
}


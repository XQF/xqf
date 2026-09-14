#include <string.h>

#include <glib.h>

#include "../src/huffman.h"

static void
roundtrip (const unsigned char *in, ssize_t inlen)
{
	/* HuffEncode only falls back to a raw (inlen+1-byte) copy *after*
	 * speculatively writing the compressed bitstream, so — matching how
	 * rcon.c itself always encodes into its fixed 64KB huffbuff rather
	 * than a buffer sized to the input — give it generous headroom
	 * instead of the tight inlen+1 a compressed-or-raw contract would
	 * otherwise suggest. */
	unsigned char *encoded = g_malloc (inlen * 4 + 1024);
	unsigned char *decoded = g_malloc (inlen > 0 ? inlen : 1);
	ssize_t enclen = 0, declen = 0;

	HuffEncode (in, encoded, inlen, &enclen);
	g_assert_cmpint (huff_failed, ==, 0);

	HuffDecode (encoded, decoded, enclen, &declen, (int) inlen);
	g_assert_cmpint (huff_failed, ==, 0);

	g_assert_cmpint (declen, ==, inlen);
	if (inlen > 0)
		g_assert_cmpmem (decoded, (size_t) declen, in, (size_t) inlen);

	g_free (encoded);
	g_free (decoded);
}

static void
test_empty_buffer_roundtrips (void)
{
	roundtrip ((const unsigned char *) "", 0);
}

static void
test_single_byte_roundtrips (void)
{
	unsigned char in[] = { 'A' };
	roundtrip (in, sizeof (in));
}

static void
test_ascii_text_roundtrips (void)
{
	const unsigned char *text = (const unsigned char *) "status\nmap dm4\nusers 3/16\n";
	roundtrip (text, (ssize_t) strlen ((const char *) text));
}

static void
test_repeated_byte_roundtrips (void)
{
	unsigned char in[200];
	memset (in, 'x', sizeof (in));
	roundtrip (in, sizeof (in));
}

static void
test_all_byte_values_roundtrip (void)
{
	unsigned char in[256];
	for (int i = 0; i < 256; i++)
		in[i] = (unsigned char) i;
	roundtrip (in, sizeof (in));
}

static void
test_decode_truncated_input_stays_within_maxlen (void)
{
	/* HuffDecode must never write past the caller-supplied maxlen, even
	 * when fed more decoded symbols than that. */
	const unsigned char *text = (const unsigned char *) "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	ssize_t inlen = (ssize_t) strlen ((const char *) text);
	unsigned char *encoded = g_malloc (inlen * 4 + 1024);
	ssize_t enclen = 0;
	HuffEncode (text, encoded, inlen, &enclen);

	unsigned char small[4];
	ssize_t declen = 0;
	HuffDecode (encoded, small, enclen, &declen, sizeof (small));
	/* declen may report more than maxlen (it counts symbols decoded before
	 * bailing out), but writing is capped — this must simply not crash
	 * under a sanitizer/valgrind run. */
	(void) declen;

	g_free (encoded);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	HuffInit ();
	g_assert_cmpint (huff_failed, ==, 0);

	g_test_add_func ("/huffman/empty-buffer-roundtrips", test_empty_buffer_roundtrips);
	g_test_add_func ("/huffman/single-byte-roundtrips", test_single_byte_roundtrips);
	g_test_add_func ("/huffman/ascii-text-roundtrips", test_ascii_text_roundtrips);
	g_test_add_func ("/huffman/repeated-byte-roundtrips", test_repeated_byte_roundtrips);
	g_test_add_func ("/huffman/all-byte-values-roundtrip", test_all_byte_values_roundtrip);
	g_test_add_func ("/huffman/decode-truncated-input-stays-within-maxlen", test_decode_truncated_input_stays_within_maxlen);

	return g_test_run ();
}

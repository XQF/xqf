#include <string.h>

#include <glib.h>

#include "../src/rcon-msg.h"

static void
test_null_input_returns_newline (void)
{
	char *out = msg_terminate (NULL, 0);
	g_assert_cmpstr (out, ==, "\n");
	g_free (out);
}

static void
test_already_terminated_is_unchanged (void)
{
	char in[] = "status\n";
	char *out = msg_terminate (in, sizeof (in));   /* includes trailing \0 */
	g_assert_cmpstr (out, ==, "status\n");
	g_free (out);
}

static void
test_appends_newline_when_null_terminated_without_one (void)
{
	char in[] = "status";
	char *out = msg_terminate (in, sizeof (in));   /* includes trailing \0, no \n */
	g_assert_cmpstr (out, ==, "status\n");
	g_free (out);
}

static void
test_appends_both_when_not_null_terminated_and_no_newline (void)
{
	char in[] = { 's', 't', 'a', 't', 'u', 's' };  /* no NUL, no newline */
	char *out = msg_terminate (in, sizeof (in));
	g_assert_cmpstr (out, ==, "status\n");
	g_free (out);
}

static void
test_appends_only_nul_when_newline_present_but_unterminated (void)
{
	char in[] = { 's', 't', 'a', 't', 'u', 's', '\n' };  /* ends in \n, no NUL */
	char *out = msg_terminate (in, sizeof (in));
	g_assert_cmpstr (out, ==, "status\n");
	g_free (out);
}

static void
test_single_byte_neither_nul_nor_newline (void)
{
	char in[] = { 'x' };
	char *out = msg_terminate (in, 1);
	g_assert_cmpstr (out, ==, "x\n");
	g_free (out);
}

static void
test_single_nul_byte_gets_newline_only (void)
{
	char in[] = { '\0' };
	char *out = msg_terminate (in, 1);
	g_assert_cmpint (strlen (out), ==, 1);
	g_assert_cmpstr (out, ==, "\n");
	g_free (out);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/rcon-msg/null-input-returns-newline", test_null_input_returns_newline);
	g_test_add_func ("/rcon-msg/already-terminated-is-unchanged", test_already_terminated_is_unchanged);
	g_test_add_func ("/rcon-msg/appends-newline-when-null-terminated-without-one", test_appends_newline_when_null_terminated_without_one);
	g_test_add_func ("/rcon-msg/appends-both-when-not-null-terminated-and-no-newline", test_appends_both_when_not_null_terminated_and_no_newline);
	g_test_add_func ("/rcon-msg/appends-only-nul-when-newline-present-but-unterminated", test_appends_only_nul_when_newline_present_but_unterminated);
	g_test_add_func ("/rcon-msg/single-byte-neither-nul-nor-newline", test_single_byte_neither_nul_nor_newline);
	g_test_add_func ("/rcon-msg/single-nul-byte-gets-newline-only", test_single_nul_byte_gets_newline_only);

	return g_test_run ();
}

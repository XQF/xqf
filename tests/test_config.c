#include <glib.h>
#include <glib/gstdio.h>

#include "../src/config.h"
#include "../src/utils.h"

static char *test_dir = NULL;

static void
test_int_roundtrip (void)
{
	config_drop_all ();
	config_set_int ("/config/TestInt/value", 42);
	g_assert_cmpint (config_get_int ("/config/TestInt/value"), ==, 42);
}

static void
test_float_roundtrip (void)
{
	config_drop_all ();
	config_set_float ("/config/TestFloat/value", 3.5);
	g_assert_cmpfloat (config_get_float ("/config/TestFloat/value"), ==, 3.5);
}

static void
test_bool_roundtrip (void)
{
	config_drop_all ();
	config_set_bool ("/config/TestBool/value", TRUE);
	g_assert_cmpint (config_get_bool ("/config/TestBool/value"), ==, TRUE);

	config_set_bool ("/config/TestBool/value", FALSE);
	g_assert_cmpint (config_get_bool ("/config/TestBool/value"), ==, FALSE);
}

static void
test_string_roundtrip (void)
{
	config_drop_all ();
	config_set_string ("/config/TestString/value", "hello world");
	char *s = config_get_string ("/config/TestString/value");
	g_assert_cmpstr (s, ==, "hello world");
	g_free (s);
}

static void
test_string_escapes (void)
{
	/* \n, \r and \\ must survive a set/get round trip unchanged, since
	 * config.c's file comment documents these as the shared escape set
	 * between the old custom parser and GKeyFile. */
	config_drop_all ();
	const char *value = "line1\nline2\rtail\\end";
	config_set_string ("/config/TestEscapes/value", value);
	char *s = config_get_string ("/config/TestEscapes/value");
	g_assert_cmpstr (s, ==, value);
	g_free (s);
}

static void
test_default_hint (void)
{
	/* "path=default" — the default hint is used only when the key is
	 * missing, and config_get_*_with_default reports it via *def. */
	config_drop_all ();

	int def = FALSE;
	int val = config_get_int_with_default ("/config/TestDefault/missing=99", &def);
	g_assert_cmpint (val, ==, 99);
	g_assert_true (def);

	config_set_int ("/config/TestDefault/missing", 7);
	val = config_get_int_with_default ("/config/TestDefault/missing=99", &def);
	g_assert_cmpint (val, ==, 7);
	g_assert_false (def);
}

static void
test_colon_in_section (void)
{
	/* GKeyFile group names may contain ':', used e.g. by the "servers"
	 * file's host:port sections. */
	config_drop_all ();
	config_set_string ("/config/Server:127.0.0.1:27500/name", "test server");
	char *s = config_get_string ("/config/Server:127.0.0.1:27500/name");
	g_assert_cmpstr (s, ==, "test server");
	g_free (s);
}

static void
test_prefix_stack (void)
{
	config_drop_all ();
	config_push_prefix ("/config/TestPrefix");
	config_set_string ("value", "prefixed");
	g_assert_cmpstr (config_get_string ("value"), ==, "prefixed");
	config_pop_prefix ();

	/* Relative paths are rejected once the prefix is popped. */
	char *s = config_get_string ("value");
	g_assert_null (s);

	s = config_get_string ("/config/TestPrefix/value");
	g_assert_cmpstr (s, ==, "prefixed");
	g_free (s);
}

static void
test_persistence (void)
{
	/* config_sync() + config_drop_all() must round-trip through disk,
	 * not just through the in-memory GKeyFile. */
	config_drop_all ();
	config_set_string ("/config/TestPersist/value", "saved to disk");
	config_sync ();
	config_drop_all ();

	char *s = config_get_string ("/config/TestPersist/value");
	g_assert_cmpstr (s, ==, "saved to disk");
	g_free (s);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	/* A single shared temp directory for the whole run: config.c's
	 * search_dirs list only ever grows (config_add_dir prepends and there
	 * is no removal API), so tests must not each add their own directory.
	 * Isolation between tests instead comes from config_drop_all() (which
	 * clears the in-memory GKeyFile so it gets re-read) plus giving every
	 * test its own section name. */
	test_dir = g_dir_make_tmp ("xqf-test-config-XXXXXX", NULL);
	g_assert_nonnull (test_dir);
	config_add_dir (test_dir);

	g_test_add_func ("/config/int-roundtrip", test_int_roundtrip);
	g_test_add_func ("/config/float-roundtrip", test_float_roundtrip);
	g_test_add_func ("/config/bool-roundtrip", test_bool_roundtrip);
	g_test_add_func ("/config/string-roundtrip", test_string_roundtrip);
	g_test_add_func ("/config/string-escapes", test_string_escapes);
	g_test_add_func ("/config/default-hint", test_default_hint);
	g_test_add_func ("/config/colon-in-section", test_colon_in_section);
	g_test_add_func ("/config/prefix-stack", test_prefix_stack);
	g_test_add_func ("/config/persistence", test_persistence);

	int result = g_test_run ();

	config_drop_all ();
	char *cfg_path = file_in_dir (test_dir, "config");
	g_unlink (cfg_path);
	g_free (cfg_path);
	g_rmdir (test_dir);
	g_free (test_dir);

	return result;
}

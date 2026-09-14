#include <glib.h>

#include "../src/server.h"

static void
test_hostname_only (void)
{
	char *addr = NULL;
	unsigned short port = 0;

	g_assert_true (parse_address ("quake.example.com", &addr, &port));
	g_assert_cmpstr (addr, ==, "quake.example.com");
	g_assert_cmpint (port, ==, 0);
	g_free (addr);
}

static void
test_ip_only (void)
{
	char *addr = NULL;
	unsigned short port = 0;

	g_assert_true (parse_address ("192.0.2.1", &addr, &port));
	g_assert_cmpstr (addr, ==, "192.0.2.1");
	g_assert_cmpint (port, ==, 0);
	g_free (addr);
}

static void
test_host_and_port (void)
{
	char *addr = NULL;
	unsigned short port = 0;

	g_assert_true (parse_address ("192.0.2.1:27500", &addr, &port));
	g_assert_cmpstr (addr, ==, "192.0.2.1");
	g_assert_cmpint (port, ==, 27500);
	g_free (addr);
}

static void
test_rejects_out_of_range_port (void)
{
	char *addr = (char *) 0x1;   /* sentinel: must be reset to NULL on failure */
	unsigned short port = 1;

	g_assert_false (parse_address ("192.0.2.1:0", &addr, &port));
	g_assert_null (addr);
	g_assert_cmpint (port, ==, 0);

	addr = (char *) 0x1;
	port = 1;
	g_assert_false (parse_address ("192.0.2.1:70000", &addr, &port));
	g_assert_null (addr);
}

static void
test_rejects_non_numeric_port (void)
{
	char *addr = NULL;
	unsigned short port = 0;

	g_assert_false (parse_address ("192.0.2.1:notaport", &addr, &port));
	g_assert_null (addr);
}

static void
test_rejects_leading_colon (void)
{
	char *addr = NULL;
	unsigned short port = 0;

	g_assert_false (parse_address (":27500", &addr, &port));
	g_assert_null (addr);
}

static void
test_rejects_null_arguments (void)
{
	char *addr = NULL;
	unsigned short port = 0;

	g_assert_false (parse_address (NULL, &addr, &port));
	g_assert_false (parse_address ("192.0.2.1", NULL, &port));
	g_assert_false (parse_address ("192.0.2.1", &addr, NULL));
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/addr-parse/hostname-only", test_hostname_only);
	g_test_add_func ("/addr-parse/ip-only", test_ip_only);
	g_test_add_func ("/addr-parse/host-and-port", test_host_and_port);
	g_test_add_func ("/addr-parse/rejects-out-of-range-port", test_rejects_out_of_range_port);
	g_test_add_func ("/addr-parse/rejects-non-numeric-port", test_rejects_non_numeric_port);
	g_test_add_func ("/addr-parse/rejects-leading-colon", test_rejects_leading_colon);
	g_test_add_func ("/addr-parse/rejects-null-arguments", test_rejects_null_arguments);

	return g_test_run ();
}

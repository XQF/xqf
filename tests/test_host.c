#include <glib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../src/defs.h"
#include "../src/host.h"

/* host.c's cache save/load path needs this; unused by these tests. */
char *user_rcdir = "/tmp";

static void
test_invalid_address (void)
{
	g_assert_null (host_add (NULL));
	g_assert_null (host_add (""));
	g_assert_null (host_add ("not-an-ip"));
	g_assert_null (host_add ("999.999.999.999"));
}

static void
test_add_dedups_by_address (void)
{
	/* host_add() does NOT ref the returned host (see server.c/xqf.c
	 * callers, which always host_ref() it themselves right away) — a new
	 * host comes back with ref_count 0, ready to be either ref'd by the
	 * caller or left to be reaped by the next unref. */
	int before = hosts_total ();

	struct host *h1 = host_add ("192.0.2.1");
	host_ref (h1);
	struct host *h2 = host_add ("192.0.2.1");
	host_ref (h2);
	struct host *h3 = host_add ("192.0.2.2");
	host_ref (h3);

	g_assert_nonnull (h1);
	g_assert_true (h1 == h2);
	g_assert_true (h1 != h3);
	g_assert_cmpint (hosts_total (), ==, before + 2);

	host_unref (h1);
	host_unref (h2);
	host_unref (h3);
}

static void
test_unref_frees_at_zero_refcount (void)
{
	int before = hosts_total ();

	struct host *h = host_add ("192.0.2.42");
	host_ref (h);
	g_assert_cmpint (hosts_total (), ==, before + 1);

	host_unref (h);
	g_assert_cmpint (hosts_total (), ==, before);
}

static void
test_all_hosts_refs_each_entry (void)
{
	/* Must own a ref before calling all_hosts(): a host resting at
	 * ref_count 0 is volatile — any balanced ref/unref pair on it (like
	 * all_hosts() + host_list_free()) drops it back through 0 and frees
	 * it, same as here if we didn't hold our own ref first. */
	struct host *h = host_add ("192.0.2.99");
	host_ref (h);
	int before = h->ref_count;

	GSList *list = all_hosts ();
	g_assert_true (g_slist_find (list, h) != NULL);
	g_assert_cmpint (h->ref_count, ==, before + 1);

	host_list_free (list);
	g_assert_cmpint (h->ref_count, ==, before);

	host_unref (h);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/host/invalid-address", test_invalid_address);
	g_test_add_func ("/host/add-dedups-by-address", test_add_dedups_by_address);
	g_test_add_func ("/host/unref-frees-at-zero-refcount", test_unref_frees_at_zero_refcount);
	g_test_add_func ("/host/all-hosts-refs-each-entry", test_all_hosts_refs_each_entry);

	return g_test_run ();
}

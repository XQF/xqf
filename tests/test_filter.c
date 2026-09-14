#include <string.h>

#include <glib.h>

#include "../src/defs.h"
#include "../src/filter-eval.h"

/* Declared in full in pref.h; filter-eval.c avoids that heavy GTK header,
 * so (like the rest of the test suite) we provide the global ourselves. */
int serverlist_countbots = 0;

static struct server_filter_vars *active_filter;

static void
install_filter (struct server_filter_vars *f)
{
	server_filters = g_array_new (FALSE, FALSE, sizeof (struct server_filter_vars *));
	g_array_append_val (server_filters, f);
	current_server_filter = 1;
	active_filter = f;
}

static void
teardown (void)
{
	current_server_filter = 0;
	if (server_filters) {
		g_array_free (server_filters, FALSE);
		server_filters = NULL;
	}
	if (active_filter) {
		server_filter_vars_free (active_filter);
		g_free (active_filter);
		active_filter = NULL;
	}
}

static struct server
make_server (void)
{
	struct server s;
	memset (&s, 0, sizeof (s));
	s.ping = 50;
	s.retries = 0;
	s.maxplayers = 16;
	s.curplayers = 4;
	return s;
}

static void
test_no_filter_passes_everything (void)
{
	struct server s = make_server ();
	current_server_filter = 0;
	server_filters = NULL;
	g_assert_true (server_pass_filter (&s));
}

static void
test_no_info_never_passes (void)
{
	struct server_filter_vars *f = server_filter_vars_new ();
	install_filter (f);

	struct server s = make_server ();
	s.ping = -1;
	g_assert_false (server_pass_filter (&s));

	teardown ();
}

static void
test_ping_and_retries (void)
{
	struct server_filter_vars *f = server_filter_vars_new ();
	f->filter_ping = 100;
	f->filter_retries = 3;
	install_filter (f);

	struct server s = make_server ();

	s.ping = 150;
	g_assert_false (server_pass_filter (&s));

	s.ping = 50;
	s.retries = 5;
	g_assert_false (server_pass_filter (&s));

	s.retries = 1;
	g_assert_true (server_pass_filter (&s));

	teardown ();
}

static void
test_not_full_and_not_empty (void)
{
	struct server_filter_vars *f = server_filter_vars_new ();
	f->filter_not_full = 1;
	f->filter_not_empty = 1;
	install_filter (f);

	struct server s = make_server ();

	s.curplayers = 16;   /* full */
	g_assert_false (server_pass_filter (&s));

	s.curplayers = 0;    /* empty */
	g_assert_false (server_pass_filter (&s));

	s.curplayers = 8;
	g_assert_true (server_pass_filter (&s));

	teardown ();
}

static void
test_cheats_and_password_flags (void)
{
	struct server_filter_vars *f = server_filter_vars_new ();
	f->filter_no_cheats = 1;
	f->filter_no_password = 1;
	install_filter (f);

	struct server s = make_server ();

	s.flags = SERVER_CHEATS;
	g_assert_false (server_pass_filter (&s));

	s.flags = SERVER_PASSWORD;
	g_assert_false (server_pass_filter (&s));

	s.flags = 0;
	g_assert_true (server_pass_filter (&s));

	teardown ();
}

static void
test_game_map_gametype_name_contains_are_case_insensitive (void)
{
	struct server_filter_vars *f = server_filter_vars_new ();
	f->game_contains = g_strdup ("QUAKE");
	f->map_contains = g_strdup ("DM4");
	f->game_type = g_strdup ("FFA");
	f->server_name_contains = g_strdup ("ARENA");
	install_filter (f);

	struct server s = make_server ();
	s.game = "quakeworld";
	s.map = "dm4-remix";
	s.gametype = "ffa-classic";
	s.name = "The Arena of Pain";
	g_assert_true (server_pass_filter (&s));

	s.game = "unrelated";
	g_assert_false (server_pass_filter (&s));

	teardown ();
}

static void
test_version_contains_reads_info_pairs (void)
{
	struct server_filter_vars *f = server_filter_vars_new ();
	f->version_contains = g_strdup ("1.5");
	install_filter (f);

	char *info_match[] = { "version", "MVDSV 1.5.0", NULL };
	struct server s = make_server ();
	s.info = info_match;
	g_assert_true (server_pass_filter (&s));

	char *info_nomatch[] = { "version", "MVDSV 2.0.0", NULL };
	s.info = info_nomatch;
	g_assert_false (server_pass_filter (&s));

	char *info_none[] = { NULL };
	s.info = info_none;
	g_assert_false (server_pass_filter (&s));

	teardown ();
}

#ifdef USE_GEOIP
static void
test_country_filter (void)
{
	struct server_filter_vars *f = server_filter_vars_new ();
	int wanted = 42;
	g_array_append_val (f->countries, wanted);
	install_filter (f);

	struct server s = make_server ();
	s.country_id = 42;
	g_assert_true (server_pass_filter (&s));

	s.country_id = 7;
	g_assert_false (server_pass_filter (&s));

	s.country_id = 0;
	g_assert_false (server_pass_filter (&s));

	teardown ();
}
#endif

static void
test_quick_filter_matches_across_fields (void)
{
	struct server s = make_server ();
	s.map = "dm4";
	s.game = "QuakeWorld";
	s.gametype = "ffa";
	s.name = "Test Server";
	char *info[] = { "hostname", "Test Server", NULL };
	s.info = info;

	filter_quick_set (NULL);
	g_assert_true (quick_filter (&s));   /* empty quick filter matches all */

	filter_quick_set ("quake");
	g_assert_true (quick_filter (&s));   /* matches s->game, case-insensitively */

	filter_quick_set ("dm4");
	g_assert_true (quick_filter (&s));   /* matches s->map */

	filter_quick_set ("nonexistent");
	g_assert_false (quick_filter (&s));

	filter_quick_set ("quake nonexistent");
	g_assert_false (quick_filter (&s));  /* every token must match */

	filter_quick_unset ();
	g_assert_null (filter_quick_get ());
	g_assert_true (quick_filter (&s));
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/filter/no-filter-passes-everything", test_no_filter_passes_everything);
	g_test_add_func ("/filter/no-info-never-passes", test_no_info_never_passes);
	g_test_add_func ("/filter/ping-and-retries", test_ping_and_retries);
	g_test_add_func ("/filter/not-full-and-not-empty", test_not_full_and_not_empty);
	g_test_add_func ("/filter/cheats-and-password-flags", test_cheats_and_password_flags);
	g_test_add_func ("/filter/game-map-gametype-name-contains-case-insensitive", test_game_map_gametype_name_contains_are_case_insensitive);
	g_test_add_func ("/filter/version-contains-reads-info-pairs", test_version_contains_reads_info_pairs);
#ifdef USE_GEOIP
	g_test_add_func ("/filter/country-filter", test_country_filter);
#endif
	g_test_add_func ("/filter/quick-filter-matches-across-fields", test_quick_filter_matches_across_fields);

	return g_test_run ();
}

#include <string.h>

#include <glib.h>

#include "../src/defs.h"
#include "../src/player-filter-eval.h"

static struct player_pattern *
add_pattern (enum pattern_mode mode, const char *pattern, unsigned groups)
{
	struct player_pattern *pp = player_pattern_new (NULL);
	pp->mode = mode;
	pp->pattern = g_strdup (pattern);
	pp->groups = groups;
	player_pattern_compile (pp);
	players = g_slist_append (players, pp);
	return pp;
}

static void
teardown (void)
{
	g_slist_foreach (players, (GFunc) free_player_pattern, NULL);
	g_slist_free (players);
	players = NULL;
}

static struct server
make_server_with_player (struct player *p)
{
	struct server s;
	memset (&s, 0, sizeof (s));
	s.players = g_slist_append (NULL, p);
	return s;
}

static void
test_no_patterns_never_passes (void)
{
	struct player p = { .name = "Anyone" };
	struct server s = make_server_with_player (&p);

	g_assert_false (player_filter (&s));

	g_slist_free (s.players);
}

static void
test_no_players_never_passes (void)
{
	add_pattern (PATTERN_MODE_STRING, "someone", PLAYER_GROUP_RED);

	struct server s;
	memset (&s, 0, sizeof (s));
	s.players = NULL;

	g_assert_false (player_filter (&s));

	teardown ();
}

static void
test_string_mode_exact_case_insensitive_match (void)
{
	add_pattern (PATTERN_MODE_STRING, "QuakeGuy", PLAYER_GROUP_RED);

	struct player p = { .name = "quakeguy" };
	struct server s = make_server_with_player (&p);
	g_assert_true (player_filter (&s));
	g_assert_cmpuint (p.flags & PLAYER_GROUP_RED, ==, PLAYER_GROUP_RED);
	g_assert_cmpuint (s.flags & PLAYER_GROUP_RED, ==, PLAYER_GROUP_RED);
	g_slist_free (s.players);

	/* STRING mode requires an exact match, not a substring */
	struct player p2 = { .name = "quakeguy2" };
	struct server s2 = make_server_with_player (&p2);
	g_assert_false (player_filter (&s2));
	g_slist_free (s2.players);

	teardown ();
}

static void
test_substr_mode_match (void)
{
	add_pattern (PATTERN_MODE_SUBSTR, "quake", PLAYER_GROUP_GREEN);

	struct player p = { .name = "TeamQuakeFan" };
	struct server s = make_server_with_player (&p);
	g_assert_true (player_filter (&s));
	g_assert_cmpuint (p.flags & PLAYER_GROUP_GREEN, ==, PLAYER_GROUP_GREEN);
	g_slist_free (s.players);

	teardown ();
}

static void
test_regexp_mode_match (void)
{
	add_pattern (PATTERN_MODE_REGEXP, "^\\[CLAN\\]", PLAYER_GROUP_BLUE);

	struct player p = { .name = "[CLAN]leader" };
	struct server s = make_server_with_player (&p);
	g_assert_true (player_filter (&s));
	g_assert_cmpuint (p.flags & PLAYER_GROUP_BLUE, ==, PLAYER_GROUP_BLUE);
	g_slist_free (s.players);

	struct player p2 = { .name = "not[CLAN]leader" };
	struct server s2 = make_server_with_player (&p2);
	g_assert_false (player_filter (&s2));
	g_slist_free (s2.players);

	teardown ();
}

static void
test_invalid_regexp_never_matches (void)
{
	/* An unbalanced bracket fails to compile; player_pattern_compile
	 * records pp->error and leaves pp->data NULL, and player_filter()
	 * skips patterns with pp->error set. */
	struct player_pattern *pp = add_pattern (PATTERN_MODE_REGEXP, "[unterminated", PLAYER_GROUP_RED);
	g_assert_nonnull (pp->error);

	struct player p = { .name = "[unterminated" };
	struct server s = make_server_with_player (&p);
	g_assert_false (player_filter (&s));
	g_slist_free (s.players);

	teardown ();
}

static void
test_flags_cleared_before_reevaluation (void)
{
	add_pattern (PATTERN_MODE_STRING, "match", PLAYER_GROUP_RED);

	struct player p = { .name = "match", .flags = PLAYER_GROUP_BLUE };
	struct server s = make_server_with_player (&p);
	s.flags = PLAYER_GROUP_GREEN;

	g_assert_true (player_filter (&s));
	g_assert_cmpuint (p.flags, ==, PLAYER_GROUP_RED);
	g_assert_cmpuint (s.flags, ==, PLAYER_GROUP_RED);

	g_slist_free (s.players);
	teardown ();
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/player-filter/no-patterns-never-passes", test_no_patterns_never_passes);
	g_test_add_func ("/player-filter/no-players-never-passes", test_no_players_never_passes);
	g_test_add_func ("/player-filter/string-mode-exact-case-insensitive-match", test_string_mode_exact_case_insensitive_match);
	g_test_add_func ("/player-filter/substr-mode-match", test_substr_mode_match);
	g_test_add_func ("/player-filter/regexp-mode-match", test_regexp_mode_match);
	g_test_add_func ("/player-filter/invalid-regexp-never-matches", test_invalid_regexp_never_matches);
	g_test_add_func ("/player-filter/flags-cleared-before-reevaluation", test_flags_cleared_before_reevaluation);

	return g_test_run ();
}

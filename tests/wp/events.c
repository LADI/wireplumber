/* WirePlumber
 *
 * Copyright © 2022 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "../common/base-test-fixture.h"

typedef struct {
  WpBaseTestFixture base;
  WpObjectManager *om;
  GPtrArray *hooks_executed;
  GPtrArray *events;
  WpTransition *transition;
} TestFixture;

static void
test_events_setup (TestFixture *self, gconstpointer user_data)
{
  wp_base_test_fixture_setup (&self->base, 0);
  self->hooks_executed = g_ptr_array_new ();
  self->events = g_ptr_array_new ();
}

static void
test_events_teardown (TestFixture *self, gconstpointer user_data)
{
  g_clear_pointer (&self->hooks_executed, g_ptr_array_unref);
  wp_base_test_fixture_teardown (&self->base);
}

#define HOOK_FUNC(x) \
  static void \
  hook_##x (WpEvent * event, TestFixture * self) \
  { \
    g_debug ("in hook_" #x); \
    g_ptr_array_add (self->hooks_executed, hook_##x); \
    g_ptr_array_add (self->events, event); \
  }

HOOK_FUNC (a)
HOOK_FUNC (b)
HOOK_FUNC (c)
HOOK_FUNC (d)

static void
hook_quit (WpEvent *event, TestFixture *self)
{
  g_debug ("in hook_quit");
  g_ptr_array_add (self->hooks_executed, hook_quit);
  g_ptr_array_add (self->events, event);
  g_main_loop_quit (self->base.loop);
}

static void
hook_after_events_with_event (WpEvent *event, TestFixture *self)
{
  g_debug ("in hook_after_events_with_event %p", event);
  g_ptr_array_add (self->hooks_executed, hook_after_events_with_event);
  g_ptr_array_add (self->events, event);
  g_main_loop_quit (self->base.loop);
}


static void
test_events_basic (TestFixture *self, gconstpointer user_data)
{
  g_autoptr (WpEventDispatcher) dispatcher = NULL;
  g_autoptr (WpEventHook) hook = NULL;
  WpEvent *event1 = NULL, *event2 = NULL;

  dispatcher = wp_event_dispatcher_get_instance (self->base.core);
  g_assert_nonnull (dispatcher);

  hook = wp_simple_event_hook_new ("hook-a", 10,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
    g_cclosure_new ((GCallback) hook_a, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook-b", -200,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
    g_cclosure_new ((GCallback) hook_b, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook-c", 100,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
      g_cclosure_new ((GCallback) hook_c, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook-d", 0,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
      g_cclosure_new ((GCallback) hook_d, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type2", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook_after_events_with_event", 2000,
    WP_EVENT_HOOK_EXEC_TYPE_AFTER_EVENTS_WITH_EVENT,
    g_cclosure_new ((GCallback) hook_after_events_with_event, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type2", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook-quit-async", 1000,
    WP_EVENT_HOOK_EXEC_TYPE_AFTER_EVENTS,
    g_cclosure_new ((GCallback) hook_quit, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type2", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  /* first event run */
  event1 = wp_event_new ("type1", 10, NULL, NULL, NULL);
  wp_event_dispatcher_push_event (dispatcher, event1);

  g_assert_cmpint (self->hooks_executed->len, == , 0);
  g_main_loop_run (self->base.loop);
  g_assert_cmpint (self->hooks_executed->len, == , 5);
  g_assert (hook_c == self->hooks_executed->pdata [0]);
  g_assert (event1 == self->events->pdata [0]);
  g_assert (hook_a == self->hooks_executed->pdata [1]);
  g_assert (event1 == self->events->pdata [1]);
  g_assert (hook_b == self->hooks_executed->pdata [2]);
  g_assert (event1 == self->events->pdata [2]);
  g_assert (hook_after_events_with_event == self->hooks_executed->pdata [3]);
  g_assert (event1 == self->events->pdata [3]);
  g_assert (hook_quit == self->hooks_executed->pdata [4]);

  g_ptr_array_remove_range (self->hooks_executed, 0, self->hooks_executed->len);
  g_assert_cmpint (self->hooks_executed->len, == , 0);
  g_ptr_array_remove_range (self->events, 0, self->events->len);
  g_assert_cmpint (self->events->len, == , 0);

  /* second event run */
  event1 = wp_event_new ("type1", 10,
    wp_properties_new ("test.prop", "some-val", NULL), NULL, NULL);
  event2 = wp_event_new ("type2", 100,
    wp_properties_new ("test.prop", "some-val", NULL), NULL, NULL);

  wp_event_dispatcher_push_event (dispatcher, event1);
  wp_event_dispatcher_push_event (dispatcher, event2);

  g_assert_cmpint (self->hooks_executed->len, == , 0);
  g_main_loop_run (self->base.loop);
  g_assert_cmpint (self->hooks_executed->len, == , 7);
  g_assert (hook_d == self->hooks_executed->pdata [0]);
  g_assert (event2 == self->events->pdata [0]);
  g_assert (hook_c == self->hooks_executed->pdata [1]);
  g_assert (event1 == self->events->pdata [1]);
  g_assert (hook_a == self->hooks_executed->pdata [2]);
  g_assert (event1 == self->events->pdata [2]);
  g_assert (hook_b == self->hooks_executed->pdata [3]);
  g_assert (event1 == self->events->pdata [3]);
  g_assert (hook_after_events_with_event == self->hooks_executed->pdata [4]);
  g_assert (event2 == self->events->pdata [4]);
  g_assert (hook_after_events_with_event == self->hooks_executed->pdata [5]);
  g_assert (event1 == self->events->pdata [5]);
  g_assert (hook_quit == self->hooks_executed->pdata [6]);

  g_ptr_array_remove_range (self->hooks_executed, 0, self->hooks_executed->len);
  g_assert_cmpint (self->hooks_executed->len, == , 0);
  g_ptr_array_remove_range (self->events, 0, self->events->len);
  g_assert_cmpint (self->events->len, == , 0);

  /* third event run */
  event1 = wp_event_new ("type1", 10,
    wp_properties_new ("test.prop", "some-val", NULL), NULL, NULL);
  event2 = wp_event_new ("type2", 100,
    wp_properties_new ("test.prop", "some-val", NULL), NULL, NULL);

  wp_event_dispatcher_push_event (dispatcher, event2);
  wp_event_dispatcher_push_event (dispatcher, event1);
  wp_event_stop_processing (event1);

  g_assert_cmpint (self->hooks_executed->len, == , 0);
  g_main_loop_run (self->base.loop);
  g_assert_cmpint (self->hooks_executed->len, == , 3);
  g_assert (hook_d == self->hooks_executed->pdata [0]);
  g_assert (event2 == self->events->pdata [0]);
  g_assert (hook_after_events_with_event == self->hooks_executed->pdata [1]);
  g_assert (event2 == self->events->pdata [1]);
  g_assert (hook_quit == self->hooks_executed->pdata [2]);
}

enum {
  STEP_ONE = WP_TRANSITION_STEP_CUSTOM_START,
  STEP_TWO,
};

static guint
async_hook_get_next_step (WpTransition *transition, guint step,
  TestFixture *self)
{
  switch (step) {
  case WP_TRANSITION_STEP_NONE: return STEP_ONE;
  case STEP_ONE: return STEP_TWO;
  case STEP_TWO: return WP_TRANSITION_STEP_NONE;
  default: return WP_TRANSITION_STEP_ERROR;
  }
}

static void
async_hook_execute_step (WpTransition *transition, guint step,
  TestFixture *self)
{
  switch (step) {
  case STEP_ONE:
    g_ptr_array_add (self->hooks_executed, async_hook_execute_step);
    self->transition = transition;
    g_main_loop_quit (self->base.loop);
    break;
  case STEP_TWO:
    self->transition = NULL;
    wp_transition_advance (transition);
    break;
  default:
    g_assert_not_reached ();
  }
}

static void
test_events_async_hook (TestFixture *self, gconstpointer user_data)
{
  g_autoptr (WpEventDispatcher) dispatcher = NULL;
  g_autoptr (WpEventHook) hook = NULL;

  dispatcher = wp_event_dispatcher_get_instance (self->base.core);
  g_assert_nonnull (dispatcher);

  hook = wp_simple_event_hook_new ("hook-a", 10,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
      g_cclosure_new ((GCallback) hook_a, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook-b", -200,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
      g_cclosure_new ((GCallback) hook_b, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook-c", 100,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
      g_cclosure_new ((GCallback) hook_c, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_simple_event_hook_new ("hook-quit", 1000,
    WP_EVENT_HOOK_EXEC_TYPE_AFTER_EVENTS,
      g_cclosure_new ((GCallback) hook_quit, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type2", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  hook = wp_async_event_hook_new ("async-test-hook", 50,
    WP_EVENT_HOOK_EXEC_TYPE_ON_EVENT,
      g_cclosure_new ((GCallback) async_hook_get_next_step, self, NULL),
      g_cclosure_new ((GCallback) async_hook_execute_step, self, NULL));
  wp_interest_event_hook_add_interest (WP_INTEREST_EVENT_HOOK (hook),
    WP_CONSTRAINT_TYPE_PW_PROPERTY, "event.type", "=s", "type1", NULL);
  wp_event_dispatcher_register_hook (dispatcher, hook);
  g_clear_object (&hook);

  wp_event_dispatcher_push_event (dispatcher,
    wp_event_new ("type1", 10, NULL, NULL, NULL));

  g_assert_cmpint (self->hooks_executed->len, == , 0);
  g_main_loop_run (self->base.loop);
  g_assert_cmpint (self->hooks_executed->len, == , 2);
  g_assert (hook_c == self->hooks_executed->pdata [0]);
  g_assert (async_hook_execute_step == self->hooks_executed->pdata [1]);

  g_assert_nonnull (self->transition);
  wp_transition_advance (self->transition);
  g_assert_null (self->transition);

  g_assert_cmpint (self->hooks_executed->len, == , 2);
  g_main_loop_run (self->base.loop);
  g_assert_cmpint (self->hooks_executed->len, == , 5);
  g_assert (hook_c == self->hooks_executed->pdata [0]);
  g_assert (async_hook_execute_step == self->hooks_executed->pdata [1]);
  g_assert (hook_a == self->hooks_executed->pdata [2]);
  g_assert (hook_b == self->hooks_executed->pdata [3]);
  g_assert (hook_quit == self->hooks_executed->pdata [4]);
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);
  wp_init (WP_INIT_ALL);

  g_test_add ("/wp/events/basic", TestFixture, NULL,
    test_events_setup, test_events_basic, test_events_teardown);
  g_test_add ("/wp/events/async_hook", TestFixture, NULL,
    test_events_setup, test_events_async_hook, test_events_teardown);

  return g_test_run ();
}

-- WirePlumber
--
-- Copyright © 2022 Collabora Ltd.
--
-- SPDX-License-Identifier: MIT
--
-- Move & follow settings handlers. If the relevant settings are enabled,
-- install hooks that will schedule a rescan of the graph when needed

local config = require ("policy-config")
local handles = {}

function handleFollowSetting (enable)
  if (not handles.follow_hook) and (enable == true) then
    handles.follow_hook = SimpleEventHook {
      name = "follow@policy-desktop",
      interests = {
        EventInterest {
          Constraint { "event.type", "=", "metadata-changed" },
          Constraint { "metadata.name", "=", "default" },
          Constraint { "event.subject.key", "c", "default.audio.source",
              "default.audio.sink", "default.video.source" },
        },
      },
      execute = function (event)
        local source = event:get_source ()
        source:call ("schedule-rescan")
      end
    }
    handles.follow_hook:register ()
  elseif (handles.follow_hook) and (enable == false) then
    handles.follow_hook:remove ()
    handles.follow_hook = nil
  end
end

function handleMoveSetting (enable)
  if (not handles.move_hook) and (enable == true) then
    handles.move_hook = SimpleEventHook {
      name = "move@policy-desktop",
      interests = {
        EventInterest {
          Constraint { "event.type", "=", "metadata-changed" },
          Constraint { "metadata.name", "=", "default" },
          Constraint { "event.subject.key", "c", "target.node", "target.object" },
        },
      },
      execute = function (event)
        local source = event:get_source ()
        source:call ("schedule-rescan")
      end
    }
    handles.move_hook:register()
  elseif (handles.move_hook) and (enable == false) then
    handles.move_hook:remove ()
    handles.move_hook = nil
  end
end

config:subscribe ("follow", handleFollowSetting)
handleFollowSetting (config.follow)

config:subscribe ("move", handleMoveSetting)
handleMoveSetting (config.move)

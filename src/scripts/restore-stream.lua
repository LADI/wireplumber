-- WirePlumber
--
-- Copyright © 2021 Collabora Ltd.
--    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
--
-- Based on restore-stream.c from pipewire-media-session
-- Copyright © 2020 Wim Taymans
--
-- SPDX-License-Identifier: MIT

-- Receive script arguments from config.lua

-- Script saves(to state file) stream properties(volume, mute, channelvolumes &
-- channel map) and if the stream happens to change "target.node" in default
-- metadata, that is also saved and when stream apears afresh, all these saved
-- properties are restored. These properties are by default remmembered
-- across(presistancy) the reboots.

-- settings file: stream-settings.conf

config_restore_props = Settings.get_boolean ("stream_default.restore-props")
    or false
config_restore_target = Settings.get_boolean ("stream_default.restore-target")
    or false

-- applies rules from stream-settings.conf when asked to
function rulesApplyProperties (properties)
  local matched, mprops = Settings.apply_rule ("stream_default", properties)

  if (matched and mprops) then
    for k, v in pairs (mprops) do
      properties [k] = v
    end
  end

end

-- the state storage
state = State ("restore-stream")
state_table = state:load ()

-- simple serializer {"foo", "bar"} -> "foo;bar;"
function serializeArray (a)
  local str = ""
  for _, v in ipairs (a) do
    str = str .. tostring (v):gsub (";", "\\;") .. ";"
  end
  return str
end

-- simple deserializer "foo;bar;" -> {"foo", "bar"}
function parseArray (str, convert_value, with_type)
  local array = {}
  local val = ""
  local escaped = false
  for i = 1, #str do
    local c = str:sub (i,i)
    if c == '\\' then
      escaped = true
    elseif c == ';' and not escaped then
      val = convert_value and convert_value (val) or val
      table.insert (array, val)
      val = ""
    else
      val = val .. tostring (c)
      escaped = false
    end
  end
  if with_type then
    array ["pod_type"] = "Array"
  end
  return array
end

function parseParam (param, id)
  local route = param:parse ()
  if route.pod_type == "Object" and route.object_id == id then
    return route.properties
  else
    return nil
  end
end

function storeAfterTimeout ()
  if timeout_source then
    timeout_source:destroy ()
  end
  timeout_source = Core.timeout_add (1000, function ()
    local saved, err = state:save (state_table)
    if not saved then
      Log.warning (err)
    end
    timeout_source = nil
  end)
end

function formKeyBase (properties)
  local keys = {
    "media.role",
    "application.id",
    "application.name",
    "media.name",
    "node.name",
  }
  local key_base = nil

  for _, k in ipairs (keys) do
    local p = properties [k]
    if p then
      key_base = string.format ("%s:%s:%s",
          properties ["media.class"]:gsub ("^Stream/", ""), k, p)
      break
    end
  end
  return key_base
end

function saveTarget (subject, target_key, type, value)
  if target_key ~= "target.node" and target_key ~= "target.object" then
    return
  end

  local node = streams_om:lookup {
    Constraint { "bound-id", "=", subject, type = "gobject" }
  }
  if not node then
    return
  end

  local stream_props = node.properties
  rulesApplyProperties (stream_props)

  if stream_props ["state.restore-target"] == "false" then
    return
  end

  local key_base = formKeyBase (stream_props)
  if not key_base then
    return
  end

  local target_value = value
  local target_name = nil

  if not target_value then
    local metadata = metadata_om:lookup ()
    if metadata then
      target_value = metadata:find (node ["bound-id"], target_key)
    end
  end
  if target_value and target_value ~= "-1" then
    local target_node
    if target_key == "target.object" then
      target_node = allnodes_om:lookup {
        Constraint { "object.serial", "=", target_value, type = "pw-global" }
      }
    else
      target_node = allnodes_om:lookup {
        Constraint { "bound-id", "=", target_value, type = "gobject" }
      }
    end
    if target_node then
      target_name = target_node.properties ["node.name"]
    end
  end
  state_table [key_base .. ":target"] = target_name

  Log.info (node, "saving stream target for " ..
    tostring (stream_props ["node.name"]) ..
    " -> " .. tostring (target_name))

  storeAfterTimeout ()
end

function restoreTarget (node, target_name)
  local target_node = allnodes_om:lookup {
    Constraint { "node.name", "=", target_name, type = "pw" }
  }

  if target_node then
    local metadata = metadata_om:lookup ()
    if metadata then
      metadata:set (node ["bound-id"], "target.node", "Spa:Id",
          target_node ["bound-id"])
    end
  end
end

function jsonTable (val, name)
  local tmp = ""
  local count = 0

  if name then tmp = tmp .. string.format ("%q", name) .. ": " end

  if type (val) == "table" then
    if val ["pod_type"] == "Array" then
      tmp = tmp .. "["
      for _, v in ipairs (val) do
	if count > 0 then tmp = tmp .. "," end
        tmp = tmp .. jsonTable (v)
	count = count + 1
      end
      tmp = tmp .. "]"
    else
      tmp = tmp .. "{"
      for k, v in pairs (val) do
	if count > 0 then tmp = tmp .. "," end
        tmp = tmp .. jsonTable (v, k)
	count = count + 1
      end
      tmp = tmp .. "}"
    end
  elseif type(val) == "number" then
    tmp = tmp .. tostring (val)
  elseif type(val) == "string" then
    tmp = tmp .. string.format ("%q", val)
  elseif type(val) == "boolean" then
    tmp = tmp .. (val and "true" or "false")
  else
    tmp = tmp .. "\"[type:" .. type (val) .. "]\""
  end
  return tmp
end

function moveToMetadata (key_base, metadata)
  local route_table = { }
  local count = 0

  key = "restore.stream." .. key_base
  key = string.gsub (key, ":", ".", 1);

  local str = state_table [key_base .. ":volume"]
  if str then
    route_table ["volume"] = tonumber (str)
    count = count + 1;
  end
  local str = state_table [key_base .. ":mute"]
  if str then
    route_table ["mute"] = str == "true"
    count = count + 1;
  end
  local str = state_table [key_base .. ":channelVolumes"]
  if str then
    route_table ["volumes"] = parseArray (str, tonumber, true)
    count = count + 1;
  end
  local str = state_table [key_base .. ":channelMap"]
  if str then
    route_table ["channels"] = parseArray (str, nil, true)
    count = count + 1;
  end

  if count > 0 then
    metadata:set (0, key, "Spa:String:JSON", jsonTable (route_table));
  end
end


function saveStream (node)
  local stream_props = node.properties
  rulesApplyProperties (stream_props)

  if config_restore_props and stream_props ["state.restore-props"] ~= "false" then
    local key_base = formKeyBase (stream_props)
    if not key_base then
      return
    end

    Log.info (node, "saving stream props for " ..
        tostring (stream_props["node.name"]))

    for p in node:iterate_params ("Props") do
      local props = parseParam (p, "Props")
      if not props then
        goto skip_prop
      end

      if props.volume then
        state_table [key_base .. ":volume"] = tostring (props.volume)
      end
      if props.mute ~= nil then
        state_table [key_base .. ":mute"] = tostring (props.mute)
      end
      if props.channelVolumes then
        state_table [key_base .. ":channelVolumes"] = serializeArray (props.channelVolumes)
      end
      if props.channelMap then
        state_table [key_base .. ":channelMap"] = serializeArray (props.channelMap)
      end

      ::skip_prop::
    end

    storeAfterTimeout ()
  end
end

function restoreStream (node)
  local stream_props = node.properties
  rulesApplyProperties (stream_props)

  local key_base = formKeyBase (stream_props)
  if not key_base then
    return
  end

  if config_restore_props and stream_props ["state.restore-props"] ~= "false" then
    local needsRestore = false
    local props = { "Spa:Pod:Object:Param:Props", "Props" }

    local str = state_table [key_base .. ":volume"]
    needsRestore = str and true or needsRestore
    props.volume = str and tonumber (str) or nil

    local str = state_table [key_base .. ":mute"]
    needsRestore = str and true or needsRestore
    props.mute = str and (str == "true") or nil

    local str = state_table [key_base .. ":channelVolumes"]
    needsRestore = str and true or needsRestore
    props.channelVolumes = str and parseArray (str, tonumber) or nil

    local str = state_table [key_base .. ":channelMap"]
    needsRestore = str and true or needsRestore
    props.channelMap = str and parseArray (str) or nil

    -- convert arrays to Spa Pod
    if props.channelVolumes then
      table.insert (props.channelVolumes, 1, "Spa:Float")
      props.channelVolumes = Pod.Array (props.channelVolumes)
    end
    if props.channelMap then
      table.insert (props.channelMap, 1, "Spa:Enum:AudioChannel")
      props.channelMap = Pod.Array (props.channelMap)
    end

    if needsRestore then
      Log.info (node, "restore values from " .. key_base)

      local param = Pod.Object (props)
      Log.debug (param, "setting props on " .. tostring (node))
      node:set_param ("Props", param)
    end
  end

  if config_restore_target and stream_props["state.restore-target"] ~= "false"
  then
    local str = state_table [key_base .. ":target"]
    if str then
      restoreTarget (node, str)
    end
  end
end

-- save "targe.node" if it is present in default metadata
if config_restore_target then
  SimpleEventHook {
    -- after default-nodes*-on-metadata-added.
    priority = 5,
    name = "restore-stream-save-target",
    type = "on-event",
    interests = {
      EventInterest {
        Constraint { "event.type", "=", "object-added" },
        Constraint { "event.subject.type", "=", "metadata" },
        Constraint { "metadata.name", "=", "default" },
      },
    },
    execute = function (event, transition)
      local metadata = event:get_subject()

      -- process existing metadata
      for s, k, t, v in metadata:iterate (Id.ANY) do
        saveTarget (s, k, t, v)
      end
    end
  }:register()

-- save "target.node" on metadata changes
  SimpleEventHook {
    -- after default-nodes*-on-metadata-changed.
    priority = 5,
    name = "restore-stream-save-target",
    type = "on-event",
    interests = {
      EventInterest {
        Constraint { "event.type", "=", "object-changed" },
        Constraint { "event.subject.type", "=", "metadata" },
        Constraint { "metadata.name", "=", "default" },
      },
    },
    execute = function (event, transition)
      local subject = event:get_subject()
      local props = event:get_properties()

      local subject_id = props["event.subject.id"]
      local key = props["event.subject.key"]
      local type = props["event.subject.spa_type"]
      local value = props["event.subject.value"]

      saveTarget (subject_id, key, type, value)
    end
  }:register()

  metadata_om = ObjectManager {
    Interest {
      type = "metadata",
      Constraint { "metadata.name", "=", "default" },
    }
  }

  -- metadata_om:connect ("object-added", function  (om, metadata)
  --   -- process existing metadata
  --   for s, k, t, v in metadata:iterate (Id.ANY) do
  --     saveTarget (s, k, t, v)
  --   end
  --   -- and watch for changes
  --   metadata:connect ("changed", function (m, subject, key, type, value)
  --     saveTarget (subject, key, type, value)
  --   end)
  -- end)
  metadata_om:activate()
end

function handleRouteSettings(subject, key, type, value)
  if type ~= "Spa:String:JSON" then
    return
  end
  if string.find(key, "^restore.stream.") == nil then
    return
  end
  if value == nil then
    return
  end
  local json = Json.Raw (value);
  if json == nil or not json:is_object () then
    return
  end

  local vparsed = json:parse ()
  local key_base = string.sub (key, string.len ("restore.stream.") + 1)
  local str;

  key_base = string.gsub (key_base, "%.", ":", 1);

  if vparsed.volume ~= nil then
    state_table [key_base .. ":volume"] = tostring (vparsed.volume)
  end
  if vparsed.mute ~= nil then
    state_table [key_base .. ":mute"] = tostring (vparsed.mute)
  end
  if vparsed.channels ~= nil then
    state_table [key_base .. ":channelMap"] = serializeArray (vparsed.channels)
  end
  if vparsed.volumes ~= nil then
    state_table [key_base .. ":channelVolumes"] = serializeArray (vparsed.volumes)
  end

  storeAfterTimeout ()
end


rs_metadata = ImplMetadata ("route-settings")
rs_metadata:activate (Features.ALL, function (m, e)
  if e then
    Log.warning ("failed to activate route-settings metadata: " .. tostring (e))
    return
  end

  -- copy state into the metadata
  moveToMetadata ("Output/Audio:media.role:Notification", m)
  -- watch for changes
  m:connect ("changed", function (m, subject, key, type, value)
    handleRouteSettings (subject, key, type, value)
  end)
end)

allnodes_om = ObjectManager { Interest { type = "node" } }
allnodes_om:activate ()

-- restore-stream properties
SimpleEventHook {
  priority = 30,
  name = "restore-stream",
  type = "on-event",
  interests = {
    EventInterest {
      Constraint { "event.type", "=", "object-added" },
      Constraint { "event.subject.type", "=", "node" },
      Constraint { "media.class", "matches", "Stream/*", type = "pw-global" },
    },
    -- and device nodes that are not associated with any routes
    EventInterest {
      Constraint { "event.type", "=", "object-added" },
      Constraint { "event.subject.type", "=", "node" },
      Constraint { "media.class", "matches", "Audio/*", type = "pw-global" },
      Constraint { "device.routes", "is-absent", type = "pw" },
    },
    EventInterest {
      Constraint { "event.type", "=", "object-added" },
      Constraint { "event.subject.type", "=", "node" },
      Constraint { "media.class", "matches", "Audio/*", type = "pw-global" },
      Constraint { "device.routes", "equals", "0", type = "pw" },
    },
  },
  execute = function (event, transition)
    restoreStream (event:get_subject())
  end
}:register()

-- save-stream if any of the stream parms changes
SimpleEventHook {
  priority = 30,
  name = "restore-stream-save-stream",
  type = "on-event",
  interests = {
    EventInterest {
      Constraint { "event.type", "=", "params-changed" },
      Constraint { "event.subject.type", "=", "node" },
      Constraint { "media.class", "matches", "Stream/*", type = "pw-global" },
    },
    -- and device nodes that are not associated with any routes
    EventInterest {
      Constraint { "event.type", "=", "params-changed" },
      Constraint { "event.subject.type", "=", "node" },
      Constraint { "media.class", "matches", "Audio/*", type = "pw-global" },
      Constraint { "device.routes", "is-absent", type = "pw" },
    },
    EventInterest {
      Constraint { "event.type", "=", "params-changed" },
      Constraint { "event.subject.type", "=", "node" },
      Constraint { "media.class", "matches", "Audio/*", type = "pw-global" },
      Constraint { "device.routes", "equals", "0", type = "pw" },
    },
  },
  execute = function (event, transition)
    saveStream (event:get_subject())
  end
}:register()

streams_om = ObjectManager {
  -- match stream nodes
  Interest {
    type = "node",
    Constraint { "media.class", "matches", "Stream/*", type = "pw-global" },
  },
  -- and device nodes that are not associated with any routes
  Interest {
    type = "node",
    Constraint { "media.class", "matches", "Audio/*", type = "pw-global" },
    Constraint { "device.routes", "is-absent", type = "pw" },
  },
  Interest {
    type = "node",
    Constraint { "media.class", "matches", "Audio/*", type = "pw-global" },
    Constraint { "device.routes", "equals", "0", type = "pw" },
  },
}
-- streams_om:connect ("object-added", function (streams_om, node)
--   node:connect ("params-changed", saveStream)
--   restoreStream (node)
-- end)
streams_om:activate ()

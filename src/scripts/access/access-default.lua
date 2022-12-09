-- WirePlumber
--
-- Copyright © 2021 Collabora Ltd.
--    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
--
-- SPDX-License-Identifier: MIT

function getDefaultPermissions (properties)
  local pw_access = properties["pipewire.access"]
  local media_category = properties["media.category"]

  if pw_access == "flatpak" and media_category == "Manager" then
    return "all"
  elseif pw_access == "flatpak" or pw_access == "restricted" then
    return "rx"
  end

  return nil
end

function getPermissions (properties)
  local matched, mprops = Settings.apply_rule ("access.rules", properties)

  if (matched and mprops["default_permissions"]) then
    return mprops["default_permissions"]
  end

  return nil
end

clients_om = ObjectManager {
  Interest { type = "client" }
}

clients_om:connect("object-added", function (om, client)
  local id = client["bound-id"]
  local properties = client["properties"]

  local perms = getPermissions (properties)
  if perms == nil then
    perms = getDefaultPermissions (properties)
  end

  if perms ~= nil then
    Log.info(client, "Granting permissions to client " .. id .. ": " .. perms)
    client:update_permissions { ["any"] = perms }
  end
end)

clients_om:activate()

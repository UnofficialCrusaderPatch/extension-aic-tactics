local M = {}

-- Bind during module enable or personality admission, before installing hooks.
function M.find(name, signature)
  local found, site = pcall(core.AOBScan, signature)
  assert(found and type(site) == 'number' and site > 0 and site % 1 == 0,
    'AIC Tactics: native ' .. name .. ' ' .. (found and 'has an invalid address' or 'not found'))
  return site
end

function M.call(site, name)
  assert(core.readByte(site) == 0xE8, 'AIC Tactics: modified native '..name..' call')
  local target = site + 5 + core.readInteger(site + 1)
  assert(target > 0, 'AIC Tactics: invalid native '..name..' call')
  return target
end

-- Validate context at an already resolved owner without scanning again.
function M.verify(site, name, signature)
  local offset=0
  for token in signature:gmatch('%S+') do
    assert(token=='?' or core.readByte(site+offset)==tonumber(token,16),
      'AIC Tactics: modified native '..name)
    offset=offset+1
  end
end

return M

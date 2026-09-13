local M = {}

-- Use the same cached discovery entry point as established UCP 3.0.7 modules.
-- Complete identifying contexts and decoded ABI/layout checks remain in each
-- binding owner. Do not follow a successful match with a full-process rescan.
function M.find(name, signature)
  local ok,site=pcall(core.AOBScan,signature)
  assert(ok and type(site)=='number' and site>0 and site%1==0,
    'AIC Tactics: missing native '..name)
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

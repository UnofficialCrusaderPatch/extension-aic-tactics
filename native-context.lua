local M = {}

-- Use UCP's cache for discovery; the bounded second scan rejects ambiguity.
-- No address fallback or per-tick cache is maintained by the extension.
function M.find(name, signature)
  local ok, site = pcall(core.AOBScan, signature)
  assert(ok and type(site) == 'number' and site > 0, 'AIC Tactics: missing native '..name)
  local second = core.scanForAOB(signature, site + 1)
  assert(second == nil or second == 0, 'AIC Tactics: ambiguous native '..name)
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

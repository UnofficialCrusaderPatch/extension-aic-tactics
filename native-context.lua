local M = {}

-- Prefer UCP's main-executable uniqueness owner when available. Stock 3.0.7
-- retains its existing cached discovery and full-process ambiguity check.
-- No address fallback or per-tick cache is maintained by the extension.
function M.find(name, signature)
  if type(core.AOBScanUnique)=='function' then
    local site=core.AOBScanUnique(signature,'AIC Tactics: '..name)
    assert(type(site)=='number' and site>0,'AIC Tactics: missing native '..name)
    return site
  end
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

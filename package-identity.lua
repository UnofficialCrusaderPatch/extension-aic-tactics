local M = {}
local function word(value)
  local bytes = {}
  for index = 1, 4 do bytes[index] = string.char(value % 256); value = math.floor(value / 256) end
  return table.concat(bytes)
end

function M.verify(identity)
  assert(type(identity) == 'table' and type(identity.sha256) == 'string' and #identity.sha256 == 64
    and type(identity.files) == 'table' and #identity.files <= 256, 'AIC Tactics: invalid package identity')
  assert(type(sha) == 'table' and type(sha.sha256) == 'function', 'AIC Tactics requires framework SHA256 support')
  local chunks, previous, total = {}, '', 0
  for _, name in ipairs(identity.files) do
    assert(type(name) == 'string' and name:match('^[%w_/.-]+$') and not name:find('..', 1, true)
      and name:sub(1,1) ~= '/' and name > previous, 'AIC Tactics: invalid package file list')
    previous = name
    local file = assert(io.open('ucp/modules/aic-tactics-0.0.1/' .. name, 'rb'))
    local data, readError = file:read(4 * 1024 * 1024 + 1)
    local closed = file:close()
    if data == nil and readError == nil then data = '' end
    assert(closed and type(data) == 'string', 'AIC Tactics: cannot read package file ' .. name)
    total = total + #data
    assert(total <= 4 * 1024 * 1024, 'AIC Tactics: package exceeds identity limit')
    chunks[#chunks + 1] = word(#name) .. name .. word(#data) .. word(0) .. data
  end
  -- Startup only: no hashing, file reads or allocation on the simulation path.
  assert(sha.sha256(table.concat(chunks)) == identity.sha256,
    'AIC Tactics: package files changed; rebuild the package before starting a match')
  return identity.sha256
end

return M

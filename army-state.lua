local M = {}
function M.new(native)
  assert(native.reserveSize == 196, 'AIC Tactics: incompatible reserve state ABI')
  local count = 9 * 49
  local function validate(values)
    assert(#values == count, 'AIC Tactics: invalid reserve state size')
    local function range(value, maximum)
      assert(value >= 0 and value <= maximum, 'AIC Tactics: invalid saved reserve state')
    end
    for player = 0, 8 do
      local start = player * 49 + 1
      range(values[start], 1)
      range(values[start + 1], 1)
      assert(values[start] + values[start + 1] <= 1, 'AIC Tactics: inconsistent saved army lifecycle')
      range(values[start + 2], 4)
      range(values[start + 3], 21)
      range(values[start + 4], 156)
      for group = 0, 21 do
        local index = start + 5 + group * 2
        range(values[index], 1249)
        if values[index] == 0 then range(values[index + 1], 0) end
      end
    end
    return values
  end
  local function rebuildIndex()
    core.setMemory(native.reserveGroupOwner, 0, 1250 * 4)
    core.setMemory(native.reserveGroupUID, 0, 1250 * 4)
    for player = 1, 8 do
      local character = core.readInteger(0x115E0F8 + player * 0x39F4)
      if character >= 2 and character <= 17
          and core.readInteger(native.configuration + (character - 1) * native.configurationSize + 316) == 1 then
        for group = 0, 21 do
          local address = native.reserves + player * 196 + 20 + group * 8
          local id, uid = core.readInteger(address), core.readInteger(address + 4)
          if id > 0 and id < 1250 then
            local record = 0x1667F78 + id * 0x334
            if core.readInteger(record + 0x34) == uid and core.readInteger(record + 0x2C) == player
                and core.readSmallInteger(record + 0x40) == 2 then
              core.writeInteger(native.reserveGroupOwner + id * 4, player)
              core.writeInteger(native.reserveGroupUID + id * 4, uid)
            end
          end
        end
      end
    end
  end
  return {wordCount = count, validate = validate,
    capture = function()
      local values = {}
      for index = 0, count - 1 do values[index + 1] = core.readInteger(native.reserves + index * 4) end
      return values
    end,
    restore = function(values)
      validate(values)
      for index, value in ipairs(values) do
        core.writeInteger(native.reserves + (index - 1) * 4, value >= 2147483648 and value - 4294967296 or value)
      end
      rebuildIndex()
    end,
    initialize = function()
      core.setMemory(native.reserves, 0, count * 4)
      rebuildIndex()
    end}
end
return M

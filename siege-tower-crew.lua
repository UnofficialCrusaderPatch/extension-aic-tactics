local M = {}

-- UnitType.hpp. Slaves, slingers and fire throwers fill remaining seats.
local priority = {
  [22]=1, [23]=1, [24]=1, [25]=1, [26]=1, [27]=1, [37]=1,
  [70]=1, [71]=2, [72]=2, [73]=1, [75]=1, [76]=2,
}

local function board(riders, towers, units)
  local nextTower = 1
  for _, group in ipairs(units) do
    for _, unit in ipairs(group) do
      if #towers == 0 then return end
      local tower = towers[nextTower]
      if riders:boardUnit(tower.id, unit) then
        tower.seats = tower.seats - 1
        if tower.seats == 0 then
          table.remove(towers, nextTower)
        else
          nextTower = nextTower + 1
        end
        if nextTower > #towers then nextTower = 1 end
      end
    end
  end
end

function M.register(riders, native, game, fallback)
  if not riders or type(riders.registerTowerCrewPolicy) ~= 'function' or not game.tribeAttackWave then return end
  local short, integer = core.readSmallInteger, core.readInteger
  local function inWave(record, player, wave)
    if short(record + 0x8C) ~= 2 or short(record + 0x96) ~= player then return false end
    local id = short(record + 0x2D8)
    if id <= 0 or id >= 1250 then return false end
    local tribe = game.tribes + id * game.tribeStride
    return short(tribe + 0x40) == 2 and integer(tribe + 0x2C) == player
      and integer(tribe + 0x34) == integer(record + 0x2E4)
      and short(tribe + game.tribeAttackWave) == wave
  end
  -- The siege-defence slots 190-191 with the native dispatcher's checks.
  local function defenceTribes(owner, player)
    local tribes = {}
    for slot = 190, 191 do
      local id = short(owner + 0x310C + slot * 2)
      local tribe = game.tribes + id * game.tribeStride
      if id > 0 and id < 1250 and integer(tribe + 0x34) == integer(owner + 0x329C + slot * 4)
          and integer(tribe + 0x2C) == player and short(tribe + 0x40) == 2 then
        tribes[id] = integer(tribe + 0x34)
      end
    end
    return tribes
  end
  local scanned = {}
  local function wallBound(record)
    return short(record + 0x39C) == 0x17 and short(record + 0x2C0) == 0x65
  end
  riders:registerTowerCrewPolicy(function(tower)
    if not riders:isClimbingReady() then return false end
    local record = game.unitRecords + tower * 0x490
    local player = short(record + 0x96)
    if player < 1 or player > 8 then return false end
    local owner = game.players + player * 0x39F4
    local character = integer(owner + 0x2300) - 1
    if character < 1 or character > 16 then return false end
    local crew = integer(native.configuration + character * native.configurationSize + 348)
    if crew == 1 or crew == 0 and not fallback then return false end
    local wave = integer(owner + 0x3924)
    if wave <= 0 or not inWave(record, player, wave) then return false end
    -- Keep seats available before departure. The owning module checks both
    -- native and replacement tower orders here, before its proximity fill.
    if not wallBound(record) or riders:getFreeSeats(tower) == 0 then return true end
    -- One scan every 32 ticks boards all wall-bound towers of the wave, at the
    -- cadence of the owning module's proximity boarding.
    local tick = integer(game.gameTick)
    if tick % 32 ~= 0 or scanned[player] == tick then return true end
    scanned[player] = tick
    local defence = defenceTribes(owner, player)
    local towers, units = {}, {{}, {}}
    for id = 1, game.unitCapacity - 1 do
      local candidate = game.unitRecords + id * 0x490
      local kind = short(candidate + 0x8E)
      if kind == 58 then
        if wallBound(candidate) and inWave(candidate, player, wave) then
          local seats = riders:getFreeSeats(id)
          if seats > 0 then towers[#towers + 1] = {id=id, seats=seats} end
        end
      elseif priority[kind] and short(candidate + 0x8C) == 2 and short(candidate + 0x96) == player then
        local uid = defence[short(candidate + 0x2D8)]
        if uid and integer(candidate + 0x2E4) == uid then
          local group = units[priority[kind]]
          group[#group + 1] = id
        end
      end
    end
    board(riders, towers, units)
    return true
  end)
end

return M

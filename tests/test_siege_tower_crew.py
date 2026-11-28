from pathlib import Path

import pytest
from lupa.lua54 import LuaRuntime


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = """
  local memory = {}
  core = {readSmallInteger=function(p) return memory[p] or 0 end}
  core.readInteger = core.readSmallInteger
  local game = {players=100000, tribes=200000, tribeStride=0x334,
    tribeAttackWave=0x2BC, unitRecords=1000, unitCapacity=24}
  local native = {configuration=300000, configurationSize=352}
  memory[game.players + 0x39F4 + 0x2300] = 2
  memory[game.players + 0x39F4 + 0x3924] = 5
  for id=1,2 do
    local tribe = game.tribes + id * game.tribeStride
    memory[tribe + 0x40], memory[tribe + 0x2C], memory[tribe + 0x34] = 2, 1, id
    memory[tribe + game.tribeAttackWave] = 5
  end
  memory[game.players + 0x39F4 + 0x310C + 190 * 2] = 1
  memory[game.players + 0x39F4 + 0x329C + 190 * 4] = 1
  local function unit(id, kind, tribe, moving)
    local p = game.unitRecords + id * 0x490
    memory[p+0x8C], memory[p+0x8E], memory[p+0x96] = 2, kind, 1
    memory[p+0x2D8], memory[p+0x2E4] = tribe, tribe
    if moving then memory[p+0x39C], memory[p+0x2C0] = 0x17, 0x65 end
  end
  unit(1, 58, 1, true)
  unit(2, 58, 1, true)
  unit(3, 58, 1, false)
  unit(4, 71, 1)
  unit(5, 30, 1)
  unit(6, 29, 1)
  unit(7, 31, 1)
  unit(8, 39, 1)
  unit(9, 22, 2)
  unit(10, 24, 1)
  unit(11, 22, 1)
  unit(12, 27, 1)
  unit(13, 75, 1)
  memory[game.unitRecords + 13 * 0x490 + 0x2E4] = 99
  local seats, calls = {[1]=1,[2]=3,[3]=3}, {}
  local riders = {
    registerTowerCrewPolicy=function(self, policy) self.policy=policy end,
    isClimbingReady=function() return scenario ~= 'climbing off' end,
    getFreeSeats=function(_, id) return seats[id] end,
    boardUnit=function(_, tower, id)
      assert(seats[tower] > 0)
      seats[tower] = seats[tower] - 1
      calls[#calls+1] = tower..':'..id
      return true
    end
  }
  local config = require('config.attacks')
  local spec = {}
  spec.AICTactics_SiegeTowerCrew = scenario ~= 'field off'
  local _, settings = config.prepare(nil, spec)
  memory[native.configuration + 352 + 348] = settings[10]
  local crew = require('siege-tower-crew')
  if scenario == 'missing module' then
    crew.register(nil, native, game)
    crew.register({}, native, game)
    return
  end
  crew.register(riders, native, game)
  if scenario == 'default' then
    assert(riders.policy(3) and #calls == 0)
    assert(riders.policy(1))
    assert(table.concat(calls, ',') == '1:10,2:11,2:12,2:4')
  else
    assert(not riders.policy(1) and #calls == 0)
  end
"""


@pytest.mark.parametrize('scenario', ['default', 'field off', 'climbing off', 'missing module'])
def test_tower_crew_policy(scenario):
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().scenario = scenario
    lua.execute("package.path = root .. '/?.lua;' .. package.path")
    lua.execute(FIXTURE)

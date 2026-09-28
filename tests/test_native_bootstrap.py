from pathlib import Path
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def runtime():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      -- This fixture isolates interval ownership. Shared native discovery has
      -- its own real-executable and failure-before-write checks.
      package.loaded['native-bindings']={initialize=function()end}
      package.loaded['config.grace']={preflight=function()end}
      package.loaded['aicTactics.dll']={configurationSize=352,configuration=0x3100000,
        game={players=0x115BDF8,rangedSortieNative=0x4CD560,meleeSortieNative=0x4CD690,
          recruitmentSites={interval=0x4D3B41,ranged=0x4D543B,melee=0x4D5443,opportunity=0x4D3BA5}}}
      package.loaded['native-recruitment']={legacyCounter=function()return 0x4100000 end}
      configFinal={}
      memory,writes,allocations={},{},{}
      memory[0x4D34AB]=0x7DA83D81;memory[0x4D34AF]=0xFE;memory[0x4D34B0]=1
      memory[0x4D34B1]=4800;memory[0x4D34B5]=0x7D;memory[0x4D34B6]=8
      local interval={0x8B,0x84,0xAA,0x64,1,0,0,0x8B,0xE8,0xF7,0xDD,0x1B,0xED,0x83,0xC5,2}
      for i,v in ipairs(interval) do memory[0x4D3B40+i]=v end
      memory[0x4D543B]=0xE8;memory[0x4D543C]=0x4CD560-0x4D5440
      memory[0x4D5443]=0xE8;memory[0x4D5444]=0x4CD690-0x4D5448
      memory[0x4D3BA5]=0x8B;memory[0x4D3BA6]=0x86;memory[0x4D3BA7]=0x115EEF0
      core={readByte=function(a)return memory[a] end,readInteger=function(a)return memory[a] end,
        AOBScan=function(pattern)
          if pattern:sub(1,5)=='57 8B' then return 0x4D5438 end
          if pattern:sub(1,5)=='8B 86' then return 0x4D3BA5 end
          if pattern:sub(1,5)=='53 55' then return 0x500180 end
          if pattern:sub(1,5)=='53 8B' then return 0x4CC840 end
          return foreignInterval and 0 or 0x4D3B41
        end,
        allocateAssembly=function(text,symbols)
          allocations[#allocations+1]=text;return 0x3000000
        end,
        writeCode=function(address,bytes)
          writes[#writes+1]=address;memory[address]=bytes[1];memory[address+1]=bytes[2]
        end}
      native=require('native').new()
    ''')
    return lua


def test_default_does_not_install_interval_override():
    runtime().execute('assert(#writes==0 and #allocations==0)')


def test_siege_payment_hooks_use_verified_owner_sites():
    runtime().execute('''
      local game=native.game
      game.siegeBuildingOwner=0x500000
      game.siegeGoldSub=0x500100
      game.siegeBuildingAdmission=0x500080
      game.siegeBuildingFailureExit=0x500090
      game.buildings=0x110700
      game.siegeGoldAddress=0x110500
      game.siegeDirectSpawn=0x500200
      game.siegePlacementFail=0x110600
      game.siegeAnglePlacedBuilding=0x110604
      game.siegeDirectExit=0x500300
      for i,byte in ipairs({0x53,0xB9,0,0,0,0,0x89,0x96,0x30,0x49,0x55,0}) do
        memory[game.siegeBuildingAdmission+i-1]=byte
      end
      memory[game.siegeGoldSub]=0x29
      memory[game.siegeGoldSub+1]=0x82
      memory[game.siegeGoldSub+2]=game.siegeGoldAddress
      for i,byte in ipairs({0x8B,0x44,0x24,0x3C,0x8B,0x7C,0x24,0x44}) do
        memory[game.siegeDirectSpawn+i-1]=byte
      end
      native.siegeResourceAdmission=0x3101000
      native.siegePaymentEnabledForGoldOffset=0x3102000
      native.preflightSiegePayment()
      native.activateSiegePayment()
      assert(#allocations==3 and #writes==3)
      native.preflightSiegePayment()
      native.activateSiegePayment()
      assert(#allocations==3 and #writes==3)
    ''')


def test_interval_override_is_explicit_and_installed_once():
    runtime().execute('''
      native.enableLegacyInterval();native.enableLegacyInterval()
      assert(#writes==1 and writes[1]==0x4D3B41 and #allocations==1)
      memory[0x4D3E6F]=0xE9;memory[0x4D3E70]=0x4000000-0x4D3E74
      memory[0x4000000]=0x0424548B;memory[0x4000004]=0x8B
      memory[0x4000005]=0x14;memory[0x4000006]=0x95;memory[0x4000007]=0x4100000
      native.preflight()
      memory[0x4D3B42]=1
      assert(not pcall(native.preflight))
    ''')


def test_legacy_or_foreign_interval_patch_rejected_before_write():
    runtime().execute('''
      memory[0x4D3B41]=0xE9
      assert(not pcall(native.enableLegacyInterval))
      assert(#writes==0 and #allocations==0)
    ''')

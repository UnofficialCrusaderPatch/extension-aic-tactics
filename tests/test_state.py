from pathlib import Path
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def state():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      memory,writes={},0
      native={configurationSize=344,configuration=10000,defenseTypeCounts=20000,
        game={gameTick=0x1FE7DA8,initialDefenseTicks=0x4D34B1,aicRecords=0x23FC8E8+676,unitCapacity=2500,buildingCapacity=2000,tribeMemberWords=157,players=0x115BDF8,tribes=0x1667F78,tribeStride=0x334},
        defenseCensusTick=23000,defenseCensusValid=23004, legacyTargetPolicy=23008,
        incidentSize=2632,combatCensusTick=24000,combatCensusValid=24004,
        combatCensus=25000,targetStates=26000,targetLifecycle=27000,incidents=30000,
        reserveSize=196,reserves=60000,reserveGroupOwner=65000,reserveGroupUID=71000,
        raidStateSize=96,raidStates=80000,raidGroupCensus=81000,
        raidUnitPower=90000,raidStaticDefenses=130000,raidBuildingCensusTick=170000,
        raidBuildingCensusValid=170004,captureIntegrity=180000,integrityDigest=180004}
      fingerprint=string.rep('a',64)
      core={readInteger=function(a)return memory[a] or 0 end,
        readString=function(a,n)
          local result={}
          for i=0,n-1,4 do
            local v=memory[a+i] or 0
            if v<0 then v=v+4294967296 end
            for b=1,4 do result[#result+1]=string.char(v%256);v=math.floor(v/256) end
          end
          return table.concat(result)
        end,
        exposeCode=function()return function()end end,
        readSmallInteger=function(a)return memory[a] or 0 end,
        writeInteger=function(a,v)memory[a]=v;writes=writes+1 end,
        setMemory=function(a,v,n)for i=0,n-1,4 do memory[a+i]=v end end}
      state=require('state').new(native,true,fingerprint)
      memory[native.configuration+4*344]=1
      memory[native.configuration+4*344+280]=1
      memory[native.defenseCensusTick]=1234
      memory[native.defenseCensusValid]=1
      memory[native.defenseTypeCounts+(80+22)*4]=7
      memory[native.defenseTypeCounts+(80+24)*4]=3
    ''')
    return lua


def test_exact_census_continuation_and_unsigned_clock():
    state().execute('''
      local bytes=state.capture()
      memory[native.defenseTypeCounts+(80+22)*4]=0
      memory[native.defenseCensusValid]=0
      state.restore(bytes)
      assert(state.capture()==bytes)
      memory[native.defenseCensusTick]=-1
      bytes=state.capture();state.restore(bytes)
      assert(memory[native.defenseCensusTick]==-1 and state.capture()==bytes)
    ''')


def test_bad_payload_and_configuration_do_not_write():
    state().execute('''
      local bytes=state.capture()
      for _,bad in ipairs({bytes:sub(2),bytes..'x','bad'}) do
        assert(not pcall(state.restore,bad) and writes==0)
      end
      memory[native.configuration+4*344+280]=0
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[native.configuration+4*344+280]=1
      memory[0x23FC8E8+676+0x184]=24
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[0x23FC8E8+676+0x184]=0
      assert(not pcall(require('state').new(native,false,fingerprint).restore,bytes) and writes==0)
      memory[0x4D34B1]=800
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[0x4D34B1]=0
      memory[native.configuration+4*344+284]=800
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[native.configuration+4*344+284]=0
      local bad=bytes:sub(1,#bytes-4)..string.char(255,255,255,127)
      assert(not pcall(state.restore,bad) and writes==0)
    ''')


def test_native_old_save_and_new_match_initialization():
    state().execute('''
      memory[0x1FE7DA8]=1000
      assert(state.callbacks.isRequired())
      assert(not pcall(state.callbacks.initialize) and writes==0)
      memory[0x1FE7DA8]=0
      state.callbacks.initialize()
      assert(memory[native.defenseCensusValid]==0)
      assert(memory[native.defenseTypeCounts+(80+22)*4]==0)
      memory[0x1FE7DA8]=1000
      memory[native.configuration+4*344]=0
      assert(not state.callbacks.isRequired())
      state.callbacks.initialize()
      assert(memory[native.defenseCensusValid]==0)
    ''')


def test_existing_map_owner_callbacks_share_the_exact_snapshot():
    state().execute('''
      local saved,path
      state.callbacks:serialize({put=function(self,p,data)path=p;saved=data end})
      memory[native.defenseTypeCounts+(80+22)*4]=0
      state.callbacks:deserialize({exists=function(self,p)return p==path end,
        get=function(self,p)assert(p==path);return saved end})
      assert(state.capture()==saved)
    ''')


def test_saved_combat_reserve_and_raid_state_survives_exactly():
    state().execute('''
      memory[native.incidents+2632]=1
      memory[native.reserves+196]=1
      memory[native.reserves+196+12]=21
      memory[native.reserves+196+16]=155
      memory[native.raidStates+96]=3
      memory[native.raidStates+96+12]=15
      memory[native.raidUnitPower+(1024+19)*4]=500
      memory[native.raidStaticDefenses+(2048+20)*4]=4
      memory[native.raidBuildingCensusTick]=1234
      memory[native.raidBuildingCensusValid]=1
      local bytes=state.capture()
      state.callbacks.initialize()
      state.restore(bytes)
      assert(state.capture()==bytes)
      assert(memory[native.raidStates+96+12]==15 and memory[native.reserves+196+16]==155)
      assert(memory[native.raidUnitPower+(1024+19)*4]==500)
    ''')


def test_invalid_late_state_and_changed_build_rejected_before_any_write():
    state().execute('''
      for _,entry in ipairs({{native.reserves+196+12,22},{native.raidStates+96+12,16},
          {native.raidStates+96+16+16,8},{native.combatCensusValid,2},
          {native.incidents+2632,2},{native.raidStaticDefenses+4096,2001}}) do
        memory[entry[1]]=entry[2]
        local invalid=state.capture()
        memory[entry[1]]=0
        writes=0
        assert(not pcall(state.restore,invalid) and writes==0)
      end
      local bytes=state.capture()
      writes=0
      assert(not pcall(require('state').new(native,true,string.rep('b',64)).restore,bytes) and writes==0)
      local captured
      state.callbacks:capture({put=function(_,_,data)captured=data end})
      assert(captured==bytes and writes==0)
    ''')


def test_native_only_optional_state_does_not_impose_new_package_requirement():
    state().execute('''
      memory[native.configuration+4*344]=0
      local bytes=state.capture()
      local reader={exists=function()return true end,get=function()return bytes end}
      local other=require('state').new(native,true,string.rep('b',64))
      other.callbacks:validate(reader)
      assert(writes==0)
      reader.required=true
      assert(not pcall(other.callbacks.validate,other.callbacks,reader) and writes==0)
      reader.exists=function()return false end
      assert(not pcall(state.callbacks.validate,state.callbacks,reader) and writes==0)
    ''')

from pathlib import Path
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def state():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      memory,writes={},0
      native={configurationSize=288,configuration=10000,defenseTypeCounts=20000,
        defenseCensusTick=23000,defenseCensusValid=23004}
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
        writeInteger=function(a,v)memory[a]=v;writes=writes+1 end,
        setMemory=function(a,v,n)for i=0,n-1,4 do memory[a+i]=v end end}
      state=require('state').new(native,true)
      memory[native.configuration+4*288]=1
      memory[native.configuration+4*288+280]=1
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
      memory[native.configuration+4*288+280]=0
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[native.configuration+4*288+280]=1
      memory[0x23FC8E8+676+0x184]=24
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[0x23FC8E8+676+0x184]=0
      assert(not pcall(require('state').new(native,false).restore,bytes) and writes==0)
      memory[0x4D34B1]=800
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[0x4D34B1]=0
      memory[native.configuration+4*288+284]=800
      assert(not pcall(state.restore,bytes) and writes==0)
      memory[native.configuration+4*288+284]=0
      local bad=bytes:sub(1,#bytes-4)..string.char(255,255,255,127)
      assert(not pcall(state.restore,bad) and writes==0)
    ''')


def test_native_old_save_and_new_match_initialization():
    state().execute('''
      memory[0x1FE7DA8]=1000
      assert(not pcall(state.callbacks.initialize) and writes==0)
      memory[0x1FE7DA8]=0
      state.callbacks.initialize()
      assert(memory[native.defenseCensusValid]==0)
      assert(memory[native.defenseTypeCounts+(80+22)*4]==0)
      memory[0x1FE7DA8]=1000
      memory[native.configuration+4*288]=0
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

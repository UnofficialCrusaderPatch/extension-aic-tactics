"""Real loader, mocked core/backend. No native/MP/save/replay acceptance."""
import os
from pathlib import Path
import pytest
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]
LOADER = Path(os.environ.get('AICLOADER_TEST_ROOT', ROOT / 'tests/deps/aicloader'))

@pytest.fixture
def lua(request):
    assert (LOADER / 'transactions.lua').is_file(), 'Set AICLOADER_TEST_ROOT to loader PR #19'
    r = LuaRuntime(unpack_returned_tuples=True)
    r.globals().module_path, r.globals().loader_path = ROOT.as_posix(), LOADER.as_posix()
    r.globals().register_provider = getattr(request, 'param', True)
    r.execute('''
      package.path=module_path..'/?.lua;'..loader_path..'/?.lua;'..package.path
      memory,writes,messages={},{},{}
      core={readInteger=function(a)return memory[a] or 0 end,
        writeInteger=function(a,v)memory[a]=v; writes[#writes+1]={a,v} end}
      package.loaded.addresses={getAIStartAddress=function(ai)return 10000+ai*676 end}
      log=function(level,message)messages[#messages+1]=message end
      hooks={registerHookCallback=function(_,callback)afterInit=callback end}
      modules={}
      loader=dofile(loader_path..'/init.lua'); loader:enable({}); afterInit()
      for ai=1,16 do
        for _,s in ipairs({'Default','Weak','Strong'}) do
          loader:setAICValue(ai,'RecruitProbDef'..s,40)
          loader:setAICValue(ai,'RecruitProbRaid'..s,20)
          loader:setAICValue(ai,'RecruitProbAttack'..s,40)
        end
      end
      backendStates,prepares,commits={},0,0
      backend={prepare=function(ai,authored,compiled)
        assert(not matchRunning,'Live unsynchronized AIC updates are unavailable')
        prepares=prepares+1
        local previous=backendStates[ai]
        return {commit=function()
          commits=commits+1; backendStates[ai]=compiled
          if failCommit then error('injected backend failure') end
        end,rollback=function()backendStates[ai]=previous end}
      end}
      provider=require('config.provider')
      if register_provider then provider.register(loader,backend) end
      writes={}
      function activate(ai)
        assert(loader:overwriteAIC(ai or 1,{RecruitPolicy='WeightedRoles',
          RecruitProbSortieDefault=10,RecruitProbDefDefault=30}))
      end
    ''')
    return r

def test_atomic_rows_use_prospective_values(lua):
    lua.execute('''
      activate()
      assert(loader:getAICValue(1,'RecruitProbDefDefault')==30)
      assert(loader:getAICValue(1,'RecruitProbSortieDefault')==10)
      assert(backendStates[1].recruitment.baseRows[1][1]==30 and backendStates[1].recruitment.baseRows[1][4]==10)
      assert(backendStates[1].recruitment.baseRows[2][1]==40)
      assert(loader:getAICValue(2,'RecruitPolicy')=='Native')
      assert(prepares==1 and commits==1)
    ''')

@pytest.mark.parametrize('policy,number', [
    ('Inherit',0),('LowestPopulation',1),('FewestTroops',2),
    ('LowestCombatPower',3),('Random',4),('LastAggressor',5),
])
def test_target_policy_defaults_and_native_storage(lua,policy,number):
    lua.execute(f'''
      local original=loader:getAICValue(1,'TargetChoice')
      assert(loader:getAICValue(1,'AttackTargetPolicy')=='Inherit')
      assert(loader:getAICValue(1,'AttackTargetCommitment')=='Default')
      assert(loader:overwriteAIC(1,{{AttackTargetPolicy='{policy}'}}))
      local compiled=backendStates[1]
      assert(compiled.schemaVersion==2 and compiled.targeting.schemaVersion==1)
      assert(compiled.targeting.policy=={number} and compiled.targeting.commitment==0)
      assert(compiled.recruitment.mode==0 and #compiled.recruitment.baseRows==0)
      assert(loader:getAICValue(1,'TargetChoice')==original)
      assert(loader:getAICValue(1,'RecruitPolicy')=='Native')
      assert(loader:getAICValue(1,'AttackTargetCommitment')=='Default')
    ''')

@pytest.mark.parametrize('spec', [
    "{AttackTargetPolicy='Balanced'}", "{AttackTargetPolicy=4}",
    "{AttackTargetCommitment='Native'}", "{AttackTargetCommitment=false}",
    "{AttackTargetCommitment='PerWave'}", "{AttackTargetCommitment=1}",
])
def test_invalid_target_fields_fail_before_either_owner(lua,spec):
    lua.execute(f'''
      assert(loader:overwriteAIC(1,{spec})==false)
      assert(prepares==0 and #writes==0)
      assert(loader:getAICValue(1,'AttackTargetPolicy')=='Inherit')
      assert(loader:getAICValue(1,'AttackTargetCommitment')=='Default')
    ''')

def test_target_commitment_partial_update_and_explicit_reset(lua):
    lua.execute('''
      assert(loader:overwriteAIC(1,{AttackTargetPolicy='Random',AttackTargetCommitment='UntilDefeated'}))
      assert(loader:overwriteAIC(1,{AttackTargetPolicy='LowestPopulation'}))
      assert(loader:getAICValue(1,'AttackTargetCommitment')=='UntilDefeated')
      assert(backendStates[1].targeting.commitment==2)
      assert(loader:overwriteAIC(1,{AttackTargetCommitment='Default'}))
      assert(backendStates[1].targeting.commitment==0)
      assert(loader:overwriteAIC(1,{AttackTargetPolicy='Inherit'}))
      local prepared=prepares
      loader:setAICValue(1,'RecruitProbDefDefault',99)
      assert(prepares==prepared and loader:getAICValue(1,'RecruitProbDefDefault')==99)
      assert(loader:overwriteAIC(1,{AttackTargetCommitment='PerAttack'}))
      assert(backendStates[1].targeting.policy==0 and backendStates[1].targeting.commitment==1)
      loader:setAICValue(1,'TargetChoice','Any')
      assert(prepares==prepared+2 and loader:getAICValue(1,'TargetChoice')==3)
    ''')

def test_target_and_recruitment_commit_and_rollback_together(lua):
    lua.execute('''
      activate()
      assert(loader:overwriteAIC(1,{AttackTargetPolicy='Random',AttackTargetCommitment='UntilDefeated'}))
      failCommit=true
      assert(loader:overwriteAIC(1,{AttackTargetPolicy='LowestPopulation',AttackTargetCommitment='Default',
        RecruitProbDefDefault=35,RecruitProbSortieDefault=5,TargetChoice='Gold'})==false)
      assert(loader:getAICValue(1,'AttackTargetPolicy')=='Random')
      assert(loader:getAICValue(1,'AttackTargetCommitment')=='UntilDefeated')
      assert(loader:getAICValue(1,'RecruitProbDefDefault')==30)
      assert(backendStates[1].targeting.policy==4 and backendStates[1].targeting.commitment==2)
      assert(backendStates[1].recruitment.baseRows[1][4]==10)
      failCommit=false
      assert(loader:overwriteAIC(1,{AttackTargetPolicy='LowestPopulation',AttackTargetCommitment='Default',
        RecruitProbDefDefault=35,RecruitProbSortieDefault=5,TargetChoice='Gold'}))
      assert(backendStates[1].targeting.policy==1 and backendStates[1].targeting.commitment==0)
      assert(backendStates[1].recruitment.baseRows[1][4]==5)
      assert(loader:getAICValue(1,'TargetChoice')==0)
    ''')

def test_target_only_does_not_validate_unused_recruitment_rows(lua):
    lua.execute('''
      loader:setAICValue(1,'RecruitProbDefDefault',99)
      assert(loader:overwriteAIC(1,{AttackTargetPolicy='Random'}))
      assert(backendStates[1].recruitment.mode==0)
      assert(loader:getAICValue(1,'RecruitProbDefDefault')==99)
      assert(loader:overwriteAIC(1,{RecruitPolicy='WeightedRoles'})==false)
      assert(backendStates[1].targeting.policy==4 and backendStates[1].recruitment.mode==0)
    ''')

def test_target_reset_clears_only_requested_character(lua):
    lua.execute('''
      activate()
      for ai=1,2 do
        assert(loader:overwriteAIC(ai,{AttackTargetPolicy='Random',AttackTargetCommitment='UntilDefeated'}))
      end
      loader:resetAIC(1)
      assert(loader:getAICValue(1,'AttackTargetPolicy')=='Inherit')
      assert(loader:getAICValue(1,'AttackTargetCommitment')=='Default')
      assert(loader:getAICValue(1,'RecruitPolicy')=='Native')
      assert(loader:getAICValue(2,'AttackTargetPolicy')=='Random')
      assert(loader:getAICValue(2,'AttackTargetCommitment')=='UntilDefeated')
      assert(backendStates[1].targeting.policy==0 and backendStates[1].targeting.commitment==0)
      assert(backendStates[1].recruitment.mode==0)
    ''')

@pytest.mark.parametrize('lua', [False], indirect=True)
def test_target_collision_unwinds_all_registration(lua):
    lua.execute('''
      loader:registerAdditionalAICValue('other','AttackTargetCommitment',function()return 'other'end,function()end)
      assert(not pcall(provider.register,loader,backend))
      for _,field in ipairs(require('config.personality').fields)do
        assert(loader:getAdditionalAICValueOwner(field)==(field=='AttackTargetCommitment' and 'other' or nil))
      end
    ''')

@pytest.mark.parametrize('spec', [
    "{RecruitPolicy='WeightedRoles',RecruitProbSortieWeak=1}",
    "{RecruitPolicy='WeightedRoles',RecruitProbSortieStrong=101}",
    "{RecruitPolicy='WeightedRoles',RecruitProbSortieDefault=0.5}",
    "{RecruitPolicy='WeightedRoles',RecruitProbSortieDefault='0'}",
    "{RecruitPolicy='WeightedRoles',RecruitProbDefDefault=20}",
    "{RecruitPolicy='WeightedRoles',RecruitProbSortieDefault=0/0}",
    "{RecruitPolicy='Native',RecruitProbSortieDefault=0}",
    "{RecruitConditions={}}", "{RecruitPolicy='WeightedRole'}",
])
def test_invalid_update_changes_neither_owner(lua,spec):
    lua.execute(f'''
      assert(loader:overwriteAIC(1,{spec})==false)
      assert(#writes==0 and prepares==0 and commits==0)
      assert(loader:getAICValue(1,'RecruitPolicy')=='Native')
      assert(loader:getAICValue(1,'RecruitProbDefDefault')==40)
    ''')

def test_condition_order_masks_and_alias_isolation(lua):
    lua.execute('''
      local rows={
        {When={Strength='Weak',HomeUnderThreat=true,AttackActive=false},Defense=65,Raid=0,Attack=10,Sortie=25},
        {When={EquipmentSurplus=true},Defense=25,Raid=15,Attack=50,Sortie=10}}
      assert(loader:overwriteAIC(1,{RecruitPolicy='WeightedRoles',RecruitConditions=rows}))
      rows[1].When.Strength='Strong'; rows[1].Defense=1
      local returned=loader:getAICValue(1,'RecruitConditions')
      assert(returned[1].Defense==65 and returned[1].When.Strength=='Weak')
      returned[1].When.HomeUnderThreat=false
      assert(loader:getAICValue(1,'RecruitConditions')[1].When.HomeUnderThreat)
      local c=backendStates[1].recruitment.conditions
      assert(c[1].strength==1 and c[1].requiredFacts==1 and c[1].forbiddenFacts==2)
      assert(c[2].strength==-1 and c[2].requiredFacts==8)
    ''')

@pytest.mark.parametrize('rows', [
    '{[2]={When={},Defense=100,Raid=0,Attack=0,Sortie=0}}',
    '{[9]={When={},Defense=100,Raid=0,Attack=0,Sortie=0}}',
    '{{When={},Defense=100,Raid=0,Attack=0}}',
    '{{When={UnverifiedThreat=true},Defense=100,Raid=0,Attack=0,Sortie=0}}',
    '{{When={HomeUnderThreat=1},Defense=100,Raid=0,Attack=0,Sortie=0}}',
    '{{When={Strength="Unknown"},Defense=100,Raid=0,Attack=0,Sortie=0}}',
    '{{When={},Defense=100,Raid=0,Attack=0,Sortie=0,Extra=1}}',
    '{{When={},Defense=100,Raid=0,Attack=0,Sortie=0},{When={},Defense=50,Raid=0,Attack=0,Sortie=0}}',
])
def test_invalid_conditions_fail_before_backend(lua,rows):
    lua.execute(f'''
      assert(loader:overwriteAIC(1,{{RecruitPolicy='WeightedRoles',RecruitConditions={rows}}})==false)
      assert(prepares==0 and #writes==0)
    ''')

def test_all_eight_conditions_and_cycles(lua):
    lua.execute('''
      local rows={}
      for i=1,8 do rows[i]={When={},Defense=100,Raid=0,Attack=0,Sortie=0} end
      assert(loader:overwriteAIC(1,{RecruitPolicy='WeightedRoles',RecruitConditions=rows}))
      assert(#backendStates[1].recruitment.conditions==8)
      local cyclic={}; cyclic[1]=cyclic
      assert(loader:overwriteAIC(1,{RecruitConditions=cyclic})==false)
      assert(#backendStates[1].recruitment.conditions==8)
    ''')

def test_partial_native_change_revalidates_active_rows(lua):
    lua.execute('''
      activate(); writes={}
      loader:setAICValue(1,'RecruitProbDefDefault',31)
      assert(#writes==0 and commits==1)
      assert(loader:overwriteAIC(1,{RecruitProbDefDefault=31,RecruitProbRaidDefault=19}))
      assert(backendStates[1].recruitment.baseRows[1][1]==31 and commits==2)
    ''')

def test_backend_failure_restores_both_owners(lua):
    lua.execute('''
      activate(); failCommit=true
      assert(loader:overwriteAIC(1,{RecruitProbDefDefault=35,RecruitProbSortieDefault=5})==false)
      assert(loader:getAICValue(1,'RecruitProbDefDefault')==30)
      assert(loader:getAICValue(1,'RecruitProbSortieDefault')==10)
      assert(backendStates[1].recruitment.baseRows[1][1]==30)
    ''')

def test_later_provider_failure_restores_committed_configuration(lua):
    lua.execute('''
      activate()
      loader:registerAICUpdateProvider('zz-failing',{
        handles=function()return true end,
        prepare=function()return {commit=function()error('later provider failure')end,rollback=function()end}end})
      assert(loader:overwriteAIC(1,{RecruitProbDefDefault=35,RecruitProbSortieDefault=5})==false)
      assert(loader:getAICValue(1,'RecruitProbDefDefault')==30)
      assert(loader:getAICValue(1,'RecruitProbSortieDefault')==10)
      assert(backendStates[1].recruitment.baseRows[1][1]==30)
    ''')

def test_native_downgrade_and_per_character_reset(lua):
    lua.execute('''
      activate(); activate(2)
      assert(loader:overwriteAIC(1,{RecruitPolicy='Native'}))
      assert(backendStates[1].recruitment.mode==0 and #backendStates[1].recruitment.baseRows==0)
      assert(loader:getAICValue(1,'RecruitProbSortieDefault')==10)
      local count=prepares
      loader:setAICValue(1,'RecruitProbDefDefault',99)
      assert(prepares==count and loader:getAICValue(1,'RecruitProbDefDefault')==99)
      assert(loader:overwriteAIC(1,{RecruitPolicy='WeightedRoles'})==false)
      loader:resetAIC(1)
      assert(loader:getAICValue(1,'RecruitProbSortieDefault')==0)
      assert(loader:getAICValue(1,'RecruitPolicy')=='Native')
      assert(loader:getAICValue(2,'RecruitPolicy')=='WeightedRoles' and backendStates[2].recruitment.mode==1)
    ''')

def test_native_legacy_handler_stays_best_effort(lua):
    lua.execute('''
      local stored=0
      loader:setAdditionalAICValue('OldField',function(ai,v)if v~=nil then stored=v end return stored end,function()end)
      loader:setAICValue(1,'OldField',7)
      loader:setAICValue(1,'RecruitProbDefDefault',99)
      assert(stored==7 and prepares==0 and loader:getAICValue(1,'RecruitProbDefDefault')==99)
    ''')

def test_backend_admission_rejects_live_update(lua):
    lua.execute('''
      activate(); writes={}; matchRunning=true
      assert(loader:overwriteAIC(1,{RecruitProbDefDefault=35,RecruitProbSortieDefault=5})==false)
      assert(#writes==0 and commits==1)
    ''')

def test_collision_preserves_installed_provider(lua):
    lua.execute('''
      assert(not pcall(provider.register,loader,backend))
      assert(loader:getAdditionalAICValueOwner('RecruitPolicy')=='aic-tactics')
      activate(); assert(commits==1)
    ''')

@pytest.mark.parametrize('lua',[False],indirect=True)
def test_late_collision_rolls_back_partial_registration(lua):
    lua.execute('''
      loader:setAdditionalAICValue('RecruitConditions',function()return 17 end,function()end)
      assert(not pcall(provider.register,loader,backend))
      assert(loader:getAdditionalAICValueOwner('RecruitPolicy')==nil)
      assert(loader:getAdditionalAICValueOwner('RecruitProbSortieStrong')==nil)
      assert(loader:getAICValue(1,'RecruitConditions')==17)
      loader:setAdditionalAICValue('RecruitConditions',nil)
      provider.register(loader,backend)
      activate(); assert(commits==1)
    ''')

"""ABI/transaction tests for the actual backend with bounded mock memory."""
from pathlib import Path
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def backend():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      memory={}
      core={readInteger=function(a)return memory[a] or 0 end,
        writeInteger=function(a,v)memory[a]=v end}
      native={configuration=10000,configurationSize=344,configurationLocked=20000,
        preflight=function()end,activate=function()end,preflightComposition=function()end,activateComposition=function()end,
        preflightCombat=function()end,activateCombat=function()end,
        preflightTargets=function()end,preflightRaids=function()end,activateRaids=function()end}
      backend=require('config.backend').new(native)
      schema=require('config.personality')
      function prepare(ai, spec)
        local authored,compiled=schema.prepare(nil,spec,function(field)
          if field:find('^RecruitProbDef') then return 100 else return 0 end
        end,false)
        return backend.prepare(ai,authored,compiled)
      end
    ''')
    return lua


def test_character_storage_commit_and_rollback():
    backend().execute('''
      local op=prepare(4,{RecruitPolicy='WeightedRoles',RecruitConditions={
        {When={AttackActive=true},Defense=70,Raid=0,Attack=0,Sortie=30}}})
      assert(next(memory)==nil)
      op.commit()
      local base=10000+4*344
      assert(memory[base]==1 and memory[base+4]==1)
      assert(memory[base+8]==-1 and memory[base+12]==2 and memory[base+16]==0)
      assert(memory[base+20]==70 and memory[base+32]==30)
      assert(memory[base+232]==100 and memory[base+248]==100 and memory[base+264]==100)
      assert(memory[10000+5*344]==nil)
      op.rollback()
      for address=base,base+340,4 do assert(memory[address]==0) end
    ''')


def test_composition_is_staged_and_rolled_back_with_the_whole_record():
    backend().execute('''
      local op=prepare(4,{RecruitPolicy='WeightedRoles',DefRecruitComposition='PreserveSlots'})
      assert(next(memory)==nil)
      op.commit()
      assert(memory[10000+4*344+280]==1)
      op.rollback()
      assert(memory[10000+4*344+280]==0)
      assert(not pcall(prepare,4,{DefRecruitComposition='PreserveSlots'}))
      assert(not pcall(prepare,4,{RecruitPolicy='WeightedRoles',DefRecruitComposition='Unknown'}))
    ''')


def test_admission_rechecked_between_prepare_and_commit():
    backend().execute('''
      local op=prepare(4,{RecruitPolicy='WeightedRoles'})
      memory[native.configurationLocked]=1
      assert(not pcall(op.commit))
      assert(memory[10000+4*344]==nil)
      assert(not pcall(prepare,4,{RecruitPolicy='WeightedRoles'}))
    ''')


def test_grace_default_bounds_and_atomic_storage():
    backend().execute('''
      local op=prepare(4,{RecruitPolicy='WeightedRoles'})
      op.commit()
      assert(memory[10000+4*344+284]==4800)
      op.rollback()
      assert(memory[10000+4*344+284]==0)
      for _,months in ipairs({0,30}) do
        prepare(4,{RecruitPolicy='WeightedRoles',RecruitInitialDefenseMonths=months}).commit()
        assert(memory[10000+4*344+284]==months*800)
      end
      for _,months in ipairs({-1,31,0.5,'6',false}) do
        assert(not pcall(prepare,4,{RecruitPolicy='WeightedRoles',RecruitInitialDefenseMonths=months}))
      end
      assert(not pcall(prepare,4,{RecruitInitialDefenseMonths=0}))
    ''')


def test_equipment_fact_both_boolean_forms_are_compiled():
    backend().execute('''
      for _,value in ipairs({true,false}) do
        prepare(4,{RecruitPolicy='WeightedRoles',RecruitConditions={
          {When={EquipmentSurplus=value},Defense=100,Raid=0,Attack=0,Sortie=0}}}).commit()
        local base=10000+4*344
        assert(memory[base+12]==(value and 8 or 0))
        assert(memory[base+16]==(value and 0 or 8))
      end
    ''')


def test_native_only_updates_keep_existing_loader_lifecycle():
    backend().execute('''
      memory[native.configurationLocked]=1
      memory[0x1FE7DA8]=5000
      prepare(4,{}).commit()
      assert(memory[10000+4*344]==0)
      assert(not pcall(prepare,4,{RecruitPolicy='WeightedRoles'}))
      memory[10000+4*344]=1
      assert(not pcall(prepare,4,{}))
      assert(not pcall(prepare,5,{}))
    ''')


def test_multiplayer_freeze_also_blocks_native_only_updates():
    backend().execute('''
      local operation=prepare(2,{})
      assert(not backend.multiplayerLocked())
      backend.freezeMultiplayer()
      assert(backend.multiplayerLocked())
      assert(not pcall(operation.commit))
      assert(not pcall(prepare,3,{}))
      assert(next(memory)==nil)
    ''')


def test_preparation_retains_authored_default_and_compiles_stable_commitment():
    backend().execute('''
      local authored, compiled=schema.prepare(nil,{AttackPreparation='DuringAttack'},function()return 0 end,false)
      assert(authored.AttackTargetPolicy=='Inherit' and authored.AttackTargetCommitment=='Default')
      assert(compiled.targeting.policy==0 and compiled.targeting.commitment==1 and compiled.preparation==1)
      local _, restored=schema.prepare(authored,{AttackPreparation='Native'},function()return 0 end,false)
      assert(restored.targeting.commitment==0 and restored.preparation==0)
      prepare(4,{AttackPreparation='DuringAttack',AttackTargetPolicy='Random',
        AttackTargetCommitment='UntilDefeated'}).commit()
      local base=10000+4*344
      assert(memory[base+288]==4 and memory[base+292]==2 and memory[base+316]==1)
      memory[native.configurationLocked]=1
      assert(not pcall(prepare,4,{AttackPreparation='Native'}))
    ''')


def test_combat_and_raid_fields_share_atomic_commit_and_rollback():
    backend().execute('''
      local op=prepare(4,{AttackTargetPolicy='LastAggressor',AttackActivation='AfterProvocation',
        ProvocationRules={ThreatPower=300,CombatTicks=40,LossPower=200,WindowTicks=320},
        RaidTargetPolicy='Opportunistic',RaidGroupCount=4,RaidMinGroupSize=8,
        RaidFocus='Food',RaidRiskTolerance='Low',RaidEnemyScope='AnyEnemy'})
      assert(next(memory)==nil)
      op.commit()
      local base=10000+4*344
      assert(memory[base+288]==5 and memory[base+296]==1)
      assert(memory[base+300]==300 and memory[base+304]==40 and memory[base+308]==200 and memory[base+312]==320)
      assert(memory[base+320]==2 and memory[base+324]==4 and memory[base+328]==8)
      assert(memory[base+332]==1 and memory[base+336]==0 and memory[base+340]==1)
      op.rollback()
      for address=base,base+340,4 do assert(memory[address]==0) end
    ''')

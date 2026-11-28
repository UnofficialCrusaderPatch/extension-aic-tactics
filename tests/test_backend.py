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
      rolePreflights,roleActivations=0,0
      native={unavailableFeatures={},configuration=10000,configurationSize=352,configurationLocked=20000,
        siegePaymentPolicy=22000,siegePaymentFallback=22080,
        siegeHarassPolicy=22100,siegeHarassMinimum=22200,
        siegeHarassFallback=22300,
        largeSiegePolicy=22400,siegeForceMaximum=22500,
        largeSiegeFallback=22600,
        patrolOrders=22700,patrolFormation=22800,patrolSpacing=22900,
        patrolOrdersFallback=23000,patrolFormationFallback=23004,patrolSpacingFallback=23008,
        lordHuntRetarget=23100,lordHuntSiege=23200,lordHuntSwitch=23300,lordHuntShare=23400,
        lordHuntSplit=23500,
        patrolExtraGroups=24100,patrolMinSize=24200,patrolRadius=24300,siegeScreen=24400,
        siegeScreenForward=24500,siegeScreenExtraGroups=24600,
        allySettings=26000,allyFallback=26800,
        game={gameTick=0x1FE7DA8,initialDefenseTicks=30000},
        admissionTick=function() return native.game.gameTick end,
        preflightLordHunt=function()huntPreflights=(huntPreflights or 0)+1 end,
        activateLordHunt=function()huntActivations=(huntActivations or 0)+1 end,
        preflight=function()end,activate=function()end,preflightComposition=function()end,activateComposition=function()end,
        preflightCombat=function()end,activateCombat=function()end,
        preflightRaids=function()end,activateRaids=function()end,
        preflightSafePlacement=function()end,activateSafePlacement=function()end,
        preflightSiegePayment=function()end,activateSiegePayment=function()end,
        preflightSiegeHarassment=function()end,activateSiegeHarassment=function()end,
        preflightLargerSiegeForce=function()end,activateLargerSiegeForce=function()end,
        preflightAttackPatrols=function()patrolPreflights=(patrolPreflights or 0)+1 end,
        activateAttackPatrols=function()patrolActivations=(patrolActivations or 0)+1 end,
        preflightSiegeScreen=function()screenPreflights=(screenPreflights or 0)+1 end,
        activateSiegeScreen=function()screenActivations=(screenActivations or 0)+1 end,
        preflightAllyRelief=function()reliefPreflights=(reliefPreflights or 0)+1 end,
        activateAllyRelief=function()reliefActivations=(reliefActivations or 0)+1 end,
        preflightAllyGoods=function()goodsPreflights=(goodsPreflights or 0)+1 end,
        activateAllyGoods=function()goodsActivations=(goodsActivations or 0)+1 end,
        preflightAllyGates=function()gatePreflights=(gatePreflights or 0)+1 end,
        activateAllyGates=function()gateActivations=(gateActivations or 0)+1 end,
        preflightAllyReliefKeep=function()keepPreflights=(keepPreflights or 0)+1 end,
        preflightEngineerRoles=function()rolePreflights=rolePreflights+1 end,
        activateEngineerRoles=function()roleActivations=roleActivations+1 end}
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
      local base=10000+4*352
      assert(memory[base]==1 and memory[base+4]==1)
      assert(memory[base+8]==-1 and memory[base+12]==2 and memory[base+16]==0)
      assert(memory[base+20]==70 and memory[base+32]==30)
      assert(memory[base+232]==100 and memory[base+248]==100 and memory[base+264]==100)
      assert(memory[10000+5*352]==nil)
      op.rollback()
      for address=base,base+348,4 do assert(memory[address]==0) end
    ''')

def test_safe_siege_record_inherits_or_overrides_module_fallback():
    backend().execute('''
      local base=10000+4*352
      local inherited=prepare(4,{})
      inherited.commit()
      assert(memory[base+344]==0)
      local explicitOff=prepare(4,{SafeSiegePlacement=false})
      explicitOff.commit()
      assert(memory[base+344]==2)
      local explicitOn=prepare(4,{SafeSiegePlacement=true})
      explicitOn.commit()
      assert(memory[base+344]==1)
      explicitOn.rollback()
      assert(memory[base+344]==2)
    ''')


def test_engineer_role_record_is_reserved_and_not_an_aic_setting():
    backend().execute('''
      local base=10000+4*352
      prepare(4,{}).commit()
      assert(memory[base+348]==0)
      assert(rolePreflights==0 and roleActivations==0)
      prepare(4,{SafeSiegePlacement=true}).commit()
      assert(memory[base+348]==0)
    ''')


def test_siege_payment_is_per_ai_and_preserves_explicit_off():
    backend().execute('''
      prepare(4,{}).commit()
      assert(memory[native.siegePaymentPolicy+16]==0)
      prepare(4,{ActualSiegeResourcePayment=false}).commit()
      prepare(5,{ActualSiegeResourcePayment=true}).commit()
      assert(memory[native.siegePaymentPolicy+16]==2)
      assert(memory[native.siegePaymentPolicy+20]==1)
      local op=prepare(4,{ActualSiegeResourcePayment=true})
      op.commit()
      assert(memory[native.siegePaymentPolicy+16]==1)
      op.rollback()
      assert(memory[native.siegePaymentPolicy+16]==2)
      assert(not pcall(prepare,4,{ActualSiegeResourcePayment=0}))
    ''')


def test_siege_harassment_preserves_absent_off_and_zero_per_ai():
    backend().execute('''
      prepare(4,{}).commit()
      assert(memory[native.siegeHarassPolicy+16]==0)
      assert(memory[native.siegeHarassMinimum+16]==0)
      prepare(4,{CoordinatedSiegeHarassment=false,SiegeHarassMinEngines=0}).commit()
      prepare(5,{CoordinatedSiegeHarassment=true,SiegeHarassMinEngines=10}).commit()
      assert(memory[native.siegeHarassPolicy+16]==2)
      assert(memory[native.siegeHarassMinimum+16]==1)
      assert(memory[native.siegeHarassPolicy+20]==1)
      assert(memory[native.siegeHarassMinimum+20]==11)
      local op=prepare(4,{CoordinatedSiegeHarassment=true})
      op.commit();op.rollback()
      assert(memory[native.siegeHarassPolicy+16]==2)
      assert(memory[native.siegeHarassMinimum+16]==1)
      for _,value in ipairs({-1,21,1.5,'3',false}) do
        assert(not pcall(prepare,4,{SiegeHarassMinEngines=value}))
      end
    ''')


def test_larger_siege_force_preserves_absent_off_and_zero_per_ai():
    backend().execute('''
      prepare(4,{}).commit()
      assert(memory[native.largeSiegePolicy+16]==0)
      assert(memory[native.siegeForceMaximum+16]==0)
      prepare(4,{LargerSiegeForces=false,SiegeForceMax=0}).commit()
      prepare(5,{LargerSiegeForces=true,SiegeForceMax=10}).commit()
      assert(memory[native.largeSiegePolicy+16]==2)
      assert(memory[native.siegeForceMaximum+16]==1)
      assert(memory[native.largeSiegePolicy+20]==1)
      assert(memory[native.siegeForceMaximum+20]==11)
      local op=prepare(4,{LargerSiegeForces=true})
      op.commit();op.rollback()
      assert(memory[native.largeSiegePolicy+16]==2)
      assert(memory[native.siegeForceMaximum+16]==1)
      for _,value in ipairs({-1,65,1.5,'10',false}) do
        assert(not pcall(prepare,4,{SiegeForceMax=value}))
      end
    ''')


def test_advanced_siege_implies_safe_placement_per_ai():
    backend().execute('''
      local base=10000+4*352
      prepare(4,{LargerSiegeForces=true}).commit()
      assert(memory[base+344]==1)
      assert(not pcall(prepare,4,{LargerSiegeForces=true,SafeSiegePlacement=false}))
      prepare(5,{CoordinatedSiegeHarassment=true}).commit()
      assert(memory[10000+5*352+344]==1)
      prepare(6,{ActualSiegeResourcePayment=true}).commit()
      assert(memory[10000+6*352+344]==1)
    ''')


def test_composition_is_staged_and_rolled_back_with_the_whole_record():
    backend().execute('''
      local op=prepare(4,{RecruitPolicy='WeightedRoles',DefRecruitComposition='PreserveSlots'})
      assert(next(memory)==nil)
      op.commit()
      assert(memory[10000+4*352+280]==1)
      op.rollback()
      assert(memory[10000+4*352+280]==0)
      assert(not pcall(prepare,4,{DefRecruitComposition='PreserveSlots'}))
      assert(not pcall(prepare,4,{RecruitPolicy='WeightedRoles',DefRecruitComposition='Unknown'}))
    ''')


def test_admission_rechecked_between_prepare_and_commit():
    backend().execute('''
      local op=prepare(4,{RecruitPolicy='WeightedRoles'})
      memory[native.configurationLocked]=1
      assert(not pcall(op.commit))
      assert(memory[10000+4*352]==nil)
      assert(not pcall(prepare,4,{RecruitPolicy='WeightedRoles'}))
    ''')


def test_grace_default_bounds_and_atomic_storage():
    backend().execute('''
      -- Unset follows the native timer as Legacy set it.
      native.game.initialDefenseTicks=30000;memory[30000]=1600
      local op=prepare(4,{RecruitPolicy='WeightedRoles'})
      op.commit()
      assert(memory[10000+4*352+284]==1600)
      op.rollback()
      assert(memory[10000+4*352+284]==0)
      for _,months in ipairs({0,30}) do
        prepare(4,{RecruitPolicy='WeightedRoles',RecruitInitialDefenseMonths=months}).commit()
        assert(memory[10000+4*352+284]==months*800)
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
        local base=10000+4*352
        assert(memory[base+12]==(value and 8 or 0))
        assert(memory[base+16]==(value and 0 or 8))
      end
    ''')


def test_native_only_updates_keep_existing_loader_lifecycle():
    backend().execute('''
      memory[native.configurationLocked]=1
      memory[0x1FE7DA8]=5000
      prepare(4,{}).commit()
      assert(memory[10000+4*352]==0)
      assert(not pcall(prepare,4,{RecruitPolicy='WeightedRoles'}))
      memory[10000+4*352]=1
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
      local base=10000+4*352
      assert(memory[base+288]==4 and memory[base+292]==2 and memory[base+316]==1)
      memory[native.configurationLocked]=1
      assert(not pcall(prepare,4,{AttackPreparation='Native'}))
    ''')


def test_combined_target_values_use_existing_native_record():
    backend().execute('''
      local authored, compiled=schema.prepare(nil,
        {AttackTargetPolicy={Choice='LowestCombatPower',UntilDefeated=true}},function()return 0 end,false)
      assert(authored.AttackTargetCommitment=='Default')
      assert(compiled.targeting.policy==3 and compiled.targeting.commitment==2)
      local op=backend.prepare(4,authored,compiled)
      op.commit()
      local base=10000+4*352
      assert(memory[base+288]==3 and memory[base+292]==2)
      op.rollback()
      local _, inherited=schema.prepare(nil,
        {AttackTargetPolicy='InheritPerAttack'},function()return 0 end,false)
      assert(inherited.targeting.policy==0 and inherited.targeting.commitment==1)
    ''')


def test_last_aggressor_policy_carries_shared_provocation_rules():
    backend().execute('''
      local authored, compiled=schema.prepare(nil,{AttackTargetPolicy={
        Choice='LastAggressor',UntilDefeated=true,Provocation={LossPower=250,WindowTicks=640}}},
        function()return 0 end,false)
      assert(compiled.targeting.policy==5 and compiled.targeting.commitment==2)
      assert(compiled.targeting.rules[1]==100 and compiled.targeting.rules[2]==200)
      assert(compiled.targeting.rules[3]==250 and compiled.targeting.rules[4]==640)
      assert(authored.ProvocationRules.LossPower==250)
      local copy=schema.copy(authored)
      copy.AttackTargetPolicy.Provocation.LossPower=1
      assert(authored.AttackTargetPolicy.Provocation.LossPower==250)
      assert(authored.AttackTargetPolicy.UntilDefeated==true)
      assert(not pcall(schema.prepare,nil,{AttackTargetPolicy={Choice='LastAggressor',
        Provocation={LossPower=250}},ProvocationRules={ThreatPower=100,
        CombatTicks=200,LossPower=100,WindowTicks=800}},function()return 0 end,false))
      assert(not pcall(schema.prepare,nil,{AttackTargetPolicy={Choice='Random',
        Provocation={LossPower=250}}},function()return 0 end,false))
    ''')


def test_combat_and_raid_fields_share_atomic_commit_and_rollback():
    backend().execute('''
      local op=prepare(4,{AttackTargetPolicy='LastAggressor',AttackActivation='AfterProvocation',
        ProvocationRules={ThreatPower=300,CombatTicks=40,LossPower=200,WindowTicks=320},
        RaidTargetPolicy='Opportunistic',RaidGroupCount=4,RaidMinGroupSize=8,
        RaidFocus='Food',RaidRiskTolerance='Low',RaidEnemyScope='AnyEnemy'})
      assert(next(memory)==nil)
      op.commit()
      local base=10000+4*352
      assert(memory[base+288]==5 and memory[base+296]==1)
      assert(memory[base+300]==300 and memory[base+304]==40 and memory[base+308]==200 and memory[base+312]==320)
      assert(memory[base+320]==2 and memory[base+324]==4 and memory[base+328]==8)
      assert(memory[base+332]==1 and memory[base+336]==0 and memory[base+340]==1)
      op.rollback()
      for address=base,base+348,4 do assert(memory[address]==0) end
    ''')


def test_random_nearby_raids_and_percentage_focus_are_per_ai():
    backend().execute('''
      local authored, compiled=schema.prepare(nil,{RaidTargetPolicy='RandomNearby',
        RaidGroupCount=4,RaidFocus={Food=55,Industry=25,HighValue=10}},
        function()return 0 end,false)
      assert(compiled.raids[1]==3 and compiled.raids[2]==4)
      assert(compiled.raids[4]==0x1000000+55+25*128+10*16384)
      assert(authored.RaidFocus.Food==55)
      local copy=schema.copy(authored)
      copy.RaidFocus.Food=0
      assert(authored.RaidFocus.Food==55)
      assert(not pcall(schema.prepare,nil,{RaidTargetPolicy='RandomNearby',
        RaidFocus={Food=60,Industry=50}},function()return 0 end,false))
      assert(not pcall(schema.prepare,nil,{RaidTargetPolicy='NearestReachable',
        RaidFocus={Food=20}},function()return 0 end,false))
    ''')


def test_patrol_fields_store_inherit_or_explicit_values_per_ai():
    backend().execute('''
      prepare(4,{}).commit()
      for _,name in ipairs({'patrolOrders','patrolFormation','patrolSpacing'}) do
        assert(memory[native[name]+16]==0)
      end
      assert(patrolPreflights==nil and patrolActivations==nil)
      prepare(4,{AICTactics_PatrolOrders='AllGroups'}).commit()
      -- The save-admission record stays identical to a native character's.
      for address=10000+4*352,10000+4*352+348,4 do assert((memory[address] or 0)==0) end
      assert(memory[native.patrolOrders+16]==2 and memory[native.patrolFormation+16]==0)
      assert(patrolPreflights==1 and patrolActivations==1)
      prepare(5,{AICTactics_PatrolOrders='Native'}).commit()
      assert(memory[native.patrolOrders+20]==1 and patrolActivations==1)
      prepare(6,{AICTactics_PatrolFormation='Mirror',AICTactics_PatrolSpacing=20}).commit()
      assert(memory[native.patrolOrders+24]==0 and memory[native.patrolFormation+24]==3)
      assert(memory[native.patrolSpacing+24]==20)
      local op=prepare(6,{AICTactics_PatrolFormation='Native'})
      op.commit()
      assert(memory[native.patrolFormation+24]==1 and memory[native.patrolSpacing+24]==0)
      op.rollback()
      assert(memory[native.patrolFormation+24]==3 and memory[native.patrolSpacing+24]==20)
    ''')


def test_patrol_field_values_are_validated():
    backend().execute('''
      for _,value in ipairs({3,41,10.5,'12',false}) do
        assert(not pcall(prepare,4,{AICTactics_PatrolSpacing=value}))
      end
      assert(not pcall(prepare,4,{AICTactics_PatrolOrders='Both'}))
      assert(not pcall(prepare,4,{AICTactics_PatrolFormation='Ring'}))
      assert(pcall(prepare,4,{AICTactics_PatrolSpacing=4}))
      assert(pcall(prepare,4,{AICTactics_PatrolSpacing=40}))
      local fallback=require('config.attacks').fallbacks
      local orders,formation,spacing,extra,minimum,radius,screen,forward=fallback(nil)
      assert(orders==0 and formation==0 and spacing==12)
      assert(extra==0 and minimum==4 and radius==12 and screen==0 and forward==4)
      orders,formation,spacing=fallback({patrolOrders='Native',patrolFormation='Mirror',patrolSpacing=20})
      assert(orders==0 and formation==2 and spacing==20)
      assert(not pcall(fallback,{patrolSpacing=3}))
      assert(select(9,fallback(nil))==0)
      local screenExtra
      orders,formation,spacing,extra,minimum,radius,screen,forward,screenExtra=fallback({patrolFormation='Routes',
        patrolExtraGroups=6,patrolMinSize=1,patrolRadius=100,siegeScreen='Native',siegeScreenForward=0,
        siegeScreenExtraGroups=6})
      assert(formation==3 and extra==6 and minimum==1 and radius==100 and screen==0 and forward==0
        and screenExtra==6)
      for _,bad in ipairs({{patrolExtraGroups=7},{patrolMinSize=0},{patrolRadius=3},{patrolRadius=101},
          {siegeScreenForward=11},{siegeScreen='Front'},{patrolExtraGroups=1.5},
          {siegeScreenExtraGroups=7}}) do
        assert(not pcall(fallback,bad))
      end
    ''')


def test_split_route_and_screen_fields_store_inherit_or_explicit_values():
    backend().execute('''
      local op=prepare(4,{AICTactics_PatrolFormation='Routes',AICTactics_PatrolExtraGroups=0,
        AICTactics_PatrolMinSize=6,AICTactics_PatrolRadius=20,AICTactics_SiegeScreen='Native',
        AICTactics_SiegeScreenForward=0})
      op.commit()
      -- The save-admission record stays identical to a native character's.
      for address=10000+4*352,10000+4*352+348,4 do assert((memory[address] or 0)==0) end
      assert(memory[native.patrolFormation+16]==4 and memory[native.patrolExtraGroups+16]==1)
      assert(memory[native.patrolMinSize+16]==7 and memory[native.patrolRadius+16]==21)
      assert(memory[native.siegeScreen+16]==1 and memory[native.siegeScreenForward+16]==1)
      assert(screenPreflights==nil and screenActivations==nil and backend.policyActive())
      op.rollback()
      for _,name in ipairs(require('config.attacks').arrays) do assert(memory[native[name]+16]==0) end
      prepare(5,{AICTactics_SiegeScreen='EngineApproach',AICTactics_PatrolFormation='Wing'}).commit()
      assert(memory[native.siegeScreen+20]==2 and memory[native.patrolFormation+20]==2)
      assert(screenPreflights==1 and screenActivations==1)
      prepare(6,{AICTactics_PatrolFormation='Native',AICTactics_PatrolExtraGroups=6}).commit()
      assert(memory[native.patrolFormation+24]==1 and memory[native.patrolExtraGroups+24]==7)
      -- Extra siege-defence groups are handed back from the combat owner's update.
      prepare(8,{AICTactics_SiegeScreenExtraGroups=2}).commit()
      assert(memory[native.siegeScreenExtraGroups+32]==3)
      for _,spec in ipairs({{AICTactics_PatrolExtraGroups=7},{AICTactics_PatrolExtraGroups=-1},
          {AICTactics_PatrolMinSize=0},{AICTactics_PatrolMinSize=51},{AICTactics_PatrolRadius=3},
          {AICTactics_PatrolRadius=101},{AICTactics_SiegeScreenForward=11},{AICTactics_SiegeScreen='Engines'},
          {AICTactics_PatrolRadius='12'},{AICTactics_PatrolFormation='Clockwise'},
          {AICTactics_SiegeScreenExtraGroups=7}}) do
        assert(not pcall(prepare,7,spec))
      end
    ''')


def test_patrol_policy_requires_a_fresh_process_like_other_policies():
    backend().execute('''
      prepare(4,{AICTactics_PatrolOrders='AllGroups'}).commit()
      assert(backend.policyActive())
      memory[native.game.gameTick]=500
      assert(not pcall(prepare,5,{AICTactics_PatrolFormation='Wing'}))
    ''')


def test_lord_hunt_fields_store_inherit_or_explicit_values_per_ai():
    backend().execute('''
      local arrays={'lordHuntRetarget','lordHuntSiege','lordHuntSwitch','lordHuntShare','lordHuntSplit'}
      prepare(4,{}).commit()
      for _,name in ipairs(arrays) do assert(memory[native[name]+16]==0) end
      assert(huntPreflights==nil and huntActivations==nil)
      prepare(4,{AICTactics_LordHuntRetarget=true,AICTactics_LordHuntSiege='HomeCastle',
        AICTactics_LordHuntSwitchPercent=0,AICTactics_LordHuntShare=30,AICTactics_LordHuntSplitShare=100}).commit()
      -- The save-admission record stays identical to a native character's.
      for address=10000+4*352,10000+4*352+348,4 do assert((memory[address] or 0)==0) end
      assert(memory[native.lordHuntRetarget+16]==2 and memory[native.lordHuntSiege+16]==2)
      assert(memory[native.lordHuntSwitch+16]==1 and memory[native.lordHuntShare+16]==31)
      assert(memory[native.lordHuntSplit+16]==101)
      assert(huntPreflights==1 and huntActivations==1)
      -- Switching a character off needs no hooks of its own.
      prepare(5,{AICTactics_LordHuntRetarget=false,AICTactics_LordHuntShare=0}).commit()
      assert(memory[native.lordHuntRetarget+20]==1 and memory[native.lordHuntShare+20]==1)
      assert(huntActivations==1)
      local op=prepare(4,{AICTactics_LordHuntSiege='HidingCastle'})
      op.commit()
      assert(memory[native.lordHuntSiege+16]==1 and memory[native.lordHuntShare+16]==0)
      op.rollback()
      assert(memory[native.lordHuntSiege+16]==2 and memory[native.lordHuntShare+16]==31)
    ''')


def test_lord_hunt_field_values_are_validated():
    backend().execute('''
      for _,field in ipairs({'AICTactics_LordHuntSwitchPercent','AICTactics_LordHuntShare',
          'AICTactics_LordHuntSplitShare'}) do
        for _,value in ipairs({-1,101,10.5,'50',true}) do assert(not pcall(prepare,4,{[field]=value})) end
        assert(pcall(prepare,4,{[field]=0}) and pcall(prepare,4,{[field]=100}))
      end
      assert(not pcall(prepare,4,{AICTactics_LordHuntRetarget='yes'}))
      assert(not pcall(prepare,4,{AICTactics_LordHuntSiege='Keep'}))
      local fallback=require('config.lord-hunt').fallbacks
      local retarget,siege,switch,share,split=fallback(nil)
      assert(retarget==0 and siege==0 and switch==80 and share==0 and split==50)
      retarget,siege,switch,share,split=fallback({lordHuntRetarget=false,lordHuntSiege='HomeCastle',
        lordHuntSwitchPercent=60,lordHuntShare=0,lordHuntSplitShare=25})
      assert(retarget==0 and siege==1 and switch==60 and share==0 and split==25)
      assert(not pcall(fallback,{lordHuntShare=101}))
      assert(not pcall(fallback,{lordHuntSiege='Home'}))
    ''')


def test_lord_hunt_policy_requires_a_fresh_process_like_other_policies():
    backend().execute('''
      prepare(4,{AICTactics_LordHuntShare=20}).commit()
      assert(backend.policyActive())
      memory[native.game.gameTick]=500
      assert(not pcall(prepare,5,{AICTactics_LordHuntRetarget=false}))
    ''')

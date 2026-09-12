from test_state import state


def test_extreme_high_unit_ids_and_saved_member_cursors_round_trip():
    state().execute('''
      native.game.unitCapacity=10000
      native.game.tribeMemberWords=625
      native.game.tribeStride=0x688
      memory[native.combatCensus+48]=1
      memory[native.combatCensus+48+4]=9999
      memory[native.combatCensus+48+8]=123
      memory[native.reserves+196+16]=624
      memory[native.raidStates+96+8]=624
      memory[native.defenseTypeCounts+(80+22)*4]=9000
      local bytes=state.capture()
      state.restore(bytes)
      assert(state.capture()==bytes)
      local before=writes
      native.game.unitCapacity=2500
      native.game.tribeMemberWords=157
      native.game.tribeStride=0x334
      assert(not pcall(state.restore,bytes) and writes==before)
    ''')


def test_extreme_cursor_past_native_membership_is_rejected_before_writes():
    state().execute('''
      native.game.unitCapacity=10000;native.game.tribeMemberWords=625
      for _,address in ipairs({native.reserves+196+16,native.raidStates+96+8}) do
        memory[address]=625
        local before=writes
        assert(not pcall(state.restore,state.capture()) and writes==before)
        memory[address]=0
      end
    ''')


def test_army_restore_uses_resolved_player_and_tribe_roots_and_stride():
    state().execute('''
      native.game.players=200000;native.game.tribes=900000
      native.game.unitCapacity=10000;native.game.tribeMemberWords=625;native.game.tribeStride=0x688
      memory[native.game.players+0x39F4+0x2300]=5
      memory[native.configuration+4*344+316]=1
      memory[native.reserves+196+20]=1249
      memory[native.reserves+196+24]=123
      local tribe=native.game.tribes+1249*0x688
      memory[tribe+0x34]=123;memory[tribe+0x2C]=1;memory[tribe+0x40]=2
      state.restore(state.capture())
      assert(memory[native.reserveGroupOwner+1249*4]==1)
      assert(memory[native.reserveGroupUID+1249*4]==123)
      memory[tribe+0x34]=456
      state.restore(state.capture())
      assert(memory[native.reserveGroupOwner+1249*4]==0)
    ''')

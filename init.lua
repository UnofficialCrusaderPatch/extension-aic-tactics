local nativeGame = require('native-bindings').resolve()
local enabled = false
return {
  enable = function(self, config)
    assert(not enabled, 'AIC Tactics requires a clean process')
    assert(type(modules.aicloader.registerAICUpdateProvider) == 'function',
      'AIC Tactics requires the transactional AIC Loader provider API')
    local mapState = assert(modules['map-extensions'], 'AIC Tactics requires Map Extensions')
    assert(type(mapState.requiredStateVersion) == 'function' and mapState:requiredStateVersion() == 1,
      'AIC Tactics requires Map Extensions 1.1.0 with required state admission')
    local identity = require('build-identity')
    require('package-identity').verify(identity)
    local native = require('native').new(nativeGame)
    native.enableNativeTargetPolicy(config and config.nativeTargetPolicy or 'Native')
    local legacyInterval = config and config.legacyRecruitInterval
    assert(legacyInterval == nil or type(legacyInterval) == 'boolean',
      'AIC Tactics: legacyRecruitInterval must be boolean')
    local nativeMonths = config and config.nativeInitialDefenseMonths
    if nativeMonths == nil then nativeMonths = 6 end
    require('config.grace').validateMonths(nativeMonths)
    if nativeMonths ~= 6 then require('config.grace').configure(nativeMonths) end
    local protocol=assert(modules.protocol,'AIC Tactics requires Protocol 1.1.0')
    assert(type(protocol.multiplayerAdmissionVersion)=='function' and protocol:multiplayerAdmissionVersion()==1,
      'AIC Tactics requires full multiplayer content admission')
    local backend=require('config.backend').new(native)
    require('config.provider').register(modules.aicloader, backend)
    if legacyInterval then native.enableLegacyInterval() end
    self.recruitmentState = require('state').new(native, legacyInterval, identity.sha256)
    protocol:registerMultiplayerAdmission('aic-tactics',function()
      backend.freezeMultiplayer()
      return sha.sha256(self.recruitmentState.identity())
    end,function(reason)
      -- The existing chat owner displays locally; do not broadcast another
      -- admission message or submit a simulation command for presentation.
      modules.chat:fireChatEvent(reason=='mismatch'
        and 'AIC Tactics: settings differ. Use the same extension files and AI settings.'
        or reason=='error' and 'AIC Tactics: settings could not be verified. Check the UCP log and restart the game.'
        or 'AIC Tactics: checking settings. Press Start again when all players have responded.',0,0)
    end)
    mapState:registerSection('aic-tactics', self.recruitmentState.callbacks,
      {required=true, format='aic-tactics-state-7', fingerprint=identity.sha256})
    log(INFO, string.format('[aic-tactics] configuration=0x%X size=%d observations=0x%X size=%d',
      native.configuration, native.configurationSize, native.observations, native.observationSize))
    local addresses = {}
    for _, name in ipairs({'combatCensus','combatCensusTick','combatCensusValid','incidents',
        'targetStates','targetLifecycle','reserves','raidStates','raidGroupCensus',
        'raidUnitPower','raidStaticDefenses','raidBuildingCensusTick','raidBuildingCensusValid'}) do
      addresses[#addresses + 1] = string.format('%s=0x%X', name, native[name])
    end
    log(INFO, '[aic-tactics] state-abi=7 ' .. table.concat(addresses, ' '))
    enabled = true
  end,
  disable = function() error('AIC Tactics requires a game restart to disable') end,
}

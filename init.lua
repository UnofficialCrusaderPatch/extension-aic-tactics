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
    local native = require('native').new()
    native.enableNativeTargetPolicy(config and config.nativeTargetPolicy or 'Native')
    local legacyInterval = config and config.legacyRecruitInterval
    assert(legacyInterval == nil or type(legacyInterval) == 'boolean',
      'AIC Tactics: legacyRecruitInterval must be boolean')
    local nativeMonths = config and config.nativeInitialDefenseMonths
    if nativeMonths == nil then nativeMonths = 6 end
    require('config.grace').validateMonths(nativeMonths)
    if nativeMonths ~= 6 then require('config.grace').configure(nativeMonths) end
    require('config.provider').register(modules.aicloader, require('config.backend').new(native))
    if legacyInterval then native.enableLegacyInterval() end
    self.recruitmentState = require('state').new(native, legacyInterval, identity.sha256)
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

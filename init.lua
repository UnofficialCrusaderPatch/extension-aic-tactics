local enabled = false
return {
  enable = function(self, config)
    assert(not enabled, 'AIC Tactics requires a clean process')
    assert(type(modules.aicloader.registerAICUpdateProvider) == 'function',
      'AIC Tactics requires the transactional AIC Loader provider API')
    local native = require('native').new()
    local legacyInterval = config and config.legacyRecruitInterval
    assert(legacyInterval == nil or type(legacyInterval) == 'boolean',
      'AIC Tactics: legacyRecruitInterval must be boolean')
    require('config.provider').register(modules.aicloader, require('config.backend').new(native))
    if legacyInterval then native.enableLegacyInterval() end
    self.recruitmentState = require('state').new(native, legacyInterval)
    assert(modules['map-extensions'], 'AIC Tactics requires Map Extensions for saved policy state')
      :registerSection('aic-tactics', self.recruitmentState.callbacks)
    log(INFO, string.format('[aic-tactics] configuration=0x%X size=%d observations=0x%X size=%d',
      native.configuration, native.configurationSize, native.observations, native.observationSize))
    enabled = true
  end,
  disable = function() error('AIC Tactics requires a game restart to disable') end,
}

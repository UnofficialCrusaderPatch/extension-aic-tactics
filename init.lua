local enabled = false
return {
  enable = function()
    assert(not enabled, 'AIC Tactics requires a clean process')
    assert(type(modules.aicloader.registerAICUpdateProvider) == 'function',
      'AIC Tactics requires the transactional AIC Loader provider API')
    local native = require('native').new()
    require('config.provider').register(modules.aicloader, require('config.backend').new(native))
    log(INFO, string.format('[aic-tactics] configuration=0x%X size=%d observations=0x%X size=%d',
      native.configuration, native.configurationSize, native.observations, native.observationSize))
    enabled = true
  end,
  disable = function() error('AIC Tactics requires a game restart to disable') end,
}

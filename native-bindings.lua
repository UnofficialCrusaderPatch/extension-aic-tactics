local M = {}
function M.initialize(native)
  assert(native.nativeBindingsSize == 24, 'AIC Tactics: incompatible native binding ABI')
  local game = require('config.grace').resolveNative()
  assert(type(modules.aicloader.getNativeAICLayout) == 'function',
    'AIC Tactics requires AIC Loader 1.1.4 native storage metadata')
  local layout = modules.aicloader:getNativeAICLayout()
  assert(layout.version == 1 and layout.characters == 16 and layout.stride == 676
      and type(layout.address) == 'number' and layout.address > 0,
    'AIC Tactics: unsupported AIC Loader native layout')
  game.aicRecords = layout.address
  local names = {'gameTick','rngState','rngValue','rngNext','initialDefenseTicks','aicRecords'}
  for index, name in ipairs(names) do
    core.writeInteger(native.nativeBindings + (index - 1) * 4, game[name])
  end
  native.game = game
end
return M

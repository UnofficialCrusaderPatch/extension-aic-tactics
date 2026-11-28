local enabled = false

local function enable(self, config)
  assert(not enabled, 'AIC Tactics requires a clean process')
  if type(config) ~= 'table' then
    if config ~= nil then log(ERROR, 'AIC Tactics settings must be a table. Using defaults.') end
    config = {}
  end
  local option = require('config.settings')
  local safePlacement = option(config, 'safeSiegePlacement', false)
  local siegePayment = option(config, 'actualSiegeResourcePayment', false)
  local siegeHarassment = option(config, 'coordinatedSiegeHarassment', false)
  local siegeHarassMinimum = option(config, 'siegeHarassMinEngines', 3, nil, 0, 20)
  local largerSiege = option(config, 'largerSiegeForces', false)
  local siegeForceMaximum = option(config, 'siegeForceMax', 0, nil, 0, 64)
  local roleCounting = option(config, 'correctEngineerRoleCounting', false)
  local towerCrew = option(config, 'siegeTowerCrew', false)
  local diagnostics = option(config, 'diagnostics', false)
  local patrols = {require('config.attacks').fallbacks(config)}
  local hunt = {require('config.lord-hunt').fallbacks(config)}
  local allies = require('config.allies').fallbacks(config)

  local loader = modules.aicloader
  assert(loader and type(loader.registerAICUpdateProvider) == 'function',
    'AIC Tactics requires the transactional AIC Loader provider API')
  local mapState = modules['map-extensions']
  if not (mapState and type(mapState.hasNativeSaveInterface) == 'function' and mapState:hasNativeSaveInterface()) then
    log(ERROR, 'AIC Tactics is off: Map Extensions save admission is unavailable.')
    return
  end
  assert(type(mapState.requiredStateVersion) == 'function' and mapState:requiredStateVersion() == 1,
    'AIC Tactics requires Map Extensions required state admission')
  local protocol = modules.protocol
  assert(protocol and type(protocol.multiplayerAdmissionVersion) == 'function'
    and protocol:multiplayerAdmissionVersion() == 1, 'AIC Tactics requires Protocol multiplayer admission')

  local native = require('native').new(require('native-bindings').resolve())
  local backend = require('config.backend').new(native)
  local version
  for _, extension in ipairs(allActiveExtensions) do
    if extension.name == 'aic-tactics' then version = extension.version end
  end
  self.recruitmentState = require('state').new(native, native.prepareState, version)
  require('config.provider').register(loader, backend)
  protocol:registerMultiplayerAdmission('aic-tactics', function()
    backend.freezeMultiplayer()
    local ok, identity = pcall(self.recruitmentState.matchIdentity)
    if ok then return sha.sha256(identity) end
  end, function(reason)
    modules.chat:fireChatEvent(reason=='mismatch'
      and 'AIC Tactics: settings differ. Use the same extension versions and AI settings.'
      or reason=='error' and 'AIC Tactics: settings could not be verified. Check the UCP log and restart the game.'
      or 'AIC Tactics: checking settings. Press Start again when all players have responded.',0,0)
  end)
  -- The owners' answers are fixed for a match, so they are read when its map or save is read.
  local callbacks = self.recruitmentState.callbacks
  for _, name in ipairs({'initialize', 'deserialize'}) do
    local read = callbacks[name]
    callbacks[name] = function(...)
      native.refreshOwnerAnswers()
      return read(...)
    end
  end
  mapState:registerSection('aic-tactics', callbacks,
    {required=true, format='aic-tactics-state-14'})

  -- The other siege features need safe placement.
  if largerSiege then
    native.preflightLargerSiegeForce()
    largerSiege = not native.unavailableFeatures.LargerSiegeForce
  end
  native.configureSafePlacement(safePlacement or siegePayment or siegeHarassment or largerSiege)
  native.configureSiegePayment(siegePayment)
  native.configureSiegeHarassment(siegeHarassment, siegeHarassMinimum)
  -- A module enabled after this one may still change the siege ranges.
  hooks.registerHookCallback('afterInit', native.finalizeSiegeRanges)
  native.configureLargerSiegeForce(largerSiege, siegeForceMaximum)
  native.configureEngineerRoles(roleCounting)
  native.configureAttackPatrols(table.unpack(patrols))
  native.configureLordHunt(table.unpack(hunt))
  native.configureAllies(allies)
  native.configureTowerCrew(towerCrew)
  if diagnostics then
    local ok, problem = pcall(function()
      core.writeInteger(native.diagnosticsLog, utils.createLuaFunctionWrapper(function(registers)
        log(INFO, core.readString(core.readInteger(registers.ESP + 4)))
        return registers
      end))
    end)
    if not ok then log(ERROR, 'AIC Tactics diagnostics is off: ' .. tostring(problem)) end
  end
  enabled = true
end

return {
  enable = function(self, config)
    local ok, problem = pcall(enable, self, config)
    if not ok then log(ERROR, 'AIC Tactics: ' .. tostring(problem)) end
  end,
  disable = function() error('AIC Tactics requires a game restart to disable') end,
}

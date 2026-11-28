local M = {}
local languages = {
  en = 'Turn ON Legacy %s and restart the game.',
  de = 'Legacy %s einschalten und das Spiel neu starten.',
  ch = '开启 Legacy %s，然后重启游戏。',
  es = 'Activa Legacy %s y reinicia el juego.',
  fa = 'گزینهٔ Legacy %s را روشن کنید و بازی را دوباره اجرا کنید.',
  fr = 'Activez Legacy %s et redémarrez le jeu.',
  hu = 'Kapcsold be a Legacy %s beállítást, majd indítsd újra a játékot.',
  ru = 'Включите Legacy %s и перезапустите игру.',
  tr = 'Legacy %s seçeneğini açın ve oyunu yeniden başlatın.',
}
local aliases = {german='de',english='en',american='en',chinese='ch',zh='ch',persian='fa',
  french='fr',spanish='es',hungarian='hu',russian='ru',turkish='tr'}
local function language()
  local value = os.getenv('UCP_GUI_LANGUAGE')
  if not value or value == '' then
    local version = (rawget(_G, 'data') or {}).version
    if type(version) == 'table' and type(version.getGameLanguage) == 'function' then
      local ok, result = pcall(version.getGameLanguage)
      if ok then value = result end
    end
  end
  value = type(value) == 'string' and value:lower():match('^[^_-]+') or 'en'
  return languages[aliases[value] or value] or languages.en
end
function M.legacyOn(option)
  return 'AIC Tactics: ' .. string.format(language(), option)
end
return M

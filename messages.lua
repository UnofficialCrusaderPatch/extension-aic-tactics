local M = {}
local languages = {
  en = {'Turn OFF Legacy %s; use %s to retain its Native behaviour. Restart the game.',
    'Turn ON Legacy %s and restart the game.'},
  de = {'Legacy %s ausschalten; das bisherige Verhalten über %s beibehalten. Spiel neu starten.',
    'Legacy %s einschalten und das Spiel neu starten.'},
  ch = {'关闭 Legacy %s；通过 %s 保留原有行为。然后重启游戏。',
    '开启 Legacy %s，然后重启游戏。'},
  es = {'Desactiva Legacy %s; usa %s para conservar su comportamiento original. Reinicia el juego.',
    'Activa Legacy %s y reinicia el juego.'},
  fa = {'گزینهٔ Legacy %s را خاموش کنید؛ برای حفظ رفتار اصلی از %s استفاده کنید. سپس بازی را دوباره اجرا کنید.',
    'گزینهٔ Legacy %s را روشن کنید و بازی را دوباره اجرا کنید.'},
  fr = {'Désactivez Legacy %s ; utilisez %s pour conserver son comportement d’origine. Redémarrez le jeu.',
    'Activez Legacy %s et redémarrez le jeu.'},
  hu = {'Kapcsold ki a Legacy %s beállítást; az eredeti működéshez használd ezt: %s. Indítsd újra a játékot.',
    'Kapcsold be a Legacy %s beállítást, majd indítsd újra a játékot.'},
  ru = {'Отключите Legacy %s; используйте %s, чтобы сохранить прежнее поведение. Перезапустите игру.',
    'Включите Legacy %s и перезапустите игру.'},
  tr = {'Legacy %s seçeneğini kapatın; özgün davranışı korumak için %s kullanın. Oyunu yeniden başlatın.',
    'Legacy %s seçeneğini açın ve oyunu yeniden başlatın.'},
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
function M.legacyOff(option, replacement)
  return 'AIC Tactics: ' .. string.format(language()[1], option, replacement)
end
function M.legacyOn(option)
  return 'AIC Tactics: ' .. string.format(language()[2], option)
end
return M

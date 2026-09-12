from pathlib import Path
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def test_legacy_actions_use_all_gui_languages_and_keep_exact_option_names():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      os.getenv=function(name)assert(name=='UCP_GUI_LANGUAGE');return language end
      local messages=require('messages')
      local seen={}
      for _,code in ipairs({'ch','de','en','es','fa','fr','hu','ru','tr'}) do
        language=code
        local off=messages.legacyOff('ai_attacktarget','nativeTargetPolicy')
        assert(off:find('ai_attacktarget',1,true) and off:find('nativeTargetPolicy',1,true))
        assert(not off:find('%%s') and not seen[off])
        assert(messages.legacyOn('ai_defense'):find('ai_defense',1,true))
        seen[off]=true
      end
      language='zh-CN'; local chinese=messages.legacyOn('ai_defense')
      language='ch'; assert(messages.legacyOn('ai_defense')==chinese)
      language='de-DE'; local german=messages.legacyOn('ai_defense')
      language='de'; assert(messages.legacyOn('ai_defense')==german)
    ''')

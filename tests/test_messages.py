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
        local on=messages.legacyOn('ai_defense')
        assert(on:find('ai_defense',1,true) and not on:find('%%s') and not seen[on])
        seen[on]=true
      end
      language='zh-CN'; local chinese=messages.legacyOn('ai_defense')
      language='ch'; assert(messages.legacyOn('ai_defense')==chinese)
      language='de-DE'; local german=messages.legacyOn('ai_defense')
      language='de'; assert(messages.legacyOn('ai_defense')==german)
    ''')

return function(config, name, default, choices, minimum, maximum)
  local value = config and config[name]
  if value == nil then return default end
  local kind = type(default)
  if type(value) ~= kind then
    log(ERROR, 'AIC Tactics: ' .. name .. ' must be a ' .. kind .. '. Using the default.')
    return default
  end
  if (choices and choices[value] == nil)
    or (minimum and (value % 1 ~= 0 or value < minimum or value > maximum)) then
    log(WARNING, 'AIC Tactics: ' .. name .. ' is outside the supported values. Using the default.')
    return default
  end
  return value
end

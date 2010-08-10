-- wrapper module to start a script with lua-akfavatar
-- if the script has: require "lua-akfavatar"

if not package.preload["lua-akfavatar"] then
  os.exit(os.execute("lua-akfavatar "..table.concat(arg, " ", 0)))
end


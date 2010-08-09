-- wrapper module to start a script with lua-akfavatar
-- if the script has: require "lua-akfavatar"

if not string.find(arg[0], "lua%-akfavatar%.lua$") then
  os.exit(os.execute("lua-akfavatar "..table.concat(arg, " ", 0)))
end


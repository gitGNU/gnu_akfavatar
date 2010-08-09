-- wrapper module to start a script with lua-akfavatar
-- if the script has: require "lua-akfavatar"

os.exit(os.execute("lua-akfavatar "..table.concat(arg, " ", 0)))


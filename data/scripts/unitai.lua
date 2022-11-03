function common(n)
    print("LUA : [" .. battle_unit.id .. "] in " .. n)
    print("LUA : [" .. battle_unit.agent_name .. "] in " .. n)
end

function UnitAILowMorale (x)
    common("UnitAILowMorale")
end

function UnitAIDefault (x)
    common("UnitAIDefault")
end

function UnitAIBehavior (x)
    common("UnitAIBehavior")
end

function UnitAIVanilla (x)
    common("UnitAIVanilla")
end

function UnitAIHardcore (x)
    common("UnitAIHardcore")
end

print("LUA : in global space")
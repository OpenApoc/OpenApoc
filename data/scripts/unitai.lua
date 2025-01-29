function common(n)
    print("LUA : [" .. battle_unit.id .. "] [" .. battle_unit.agent_name .. "] in " .. n)
end

function UnitAILowMorale (x)
    common("UnitAILowMorale")
    print("LUA : [" .. battle_unit.ai_type .. "]")
end

--function UnitAIDefault (x)
--    common("UnitAIDefault")
--end

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
local function Input()
   return true
end

states = {}

states.Stand = 
{
    animation = 'foo',
    condition = function() if Input() then return 'ToWalk' end
    end
}

states.ToWalk = 
{
    animation = 'foo',
    condition = function() if Input() then return 'Blargh' end
    end
}

states.Active = states.Stand

local function do_fsm()
    repeat
          
           local nextState = states.Active.condition()
           if nextState ~= nil then
              print(nextState)
              states.Active = states[nextState]
           end
    until states.Active == nil
end

do_fsm()

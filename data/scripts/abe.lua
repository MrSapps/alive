-- TODO: Engine bindings
local function InputLeft()
   return true
end

local function InputRight()
   return true
end

local function InputUp()
   return true
end

local function InputDown()
   return true
end

local function InputChant()
   return true
end

local function InputHop()
   return true
end

local function InputSneak()
   return true
end

local function InputHop()
   return true
end

local function InputAction()
   return true
end

local function Direction()
    return 0
end

local function FlipDirection()
end

-- Composite helpers
local function InputSameAsDirection()
    return (InputLeft() and Direction() == kLeft) or (InputRight() and Direction() == kRight)
end

local function InputNotSameAsDirection()
    return (InputLeft() and Direction() == kRight) or (InputRight() and Direction() == kLeft)
end

local kLeft = 0
local kRight = 1

local states = {}

states.Stand = 
{
    animation = 'AbeStandIdle',
    condition = function(s)
        if (InputDown()) then
            -- ToHoistDown
            return 'ToCrouch'
        end
        if (InputUp()) then
            -- ToHoistUp
            return 'ToJump'
        end
        if (InputAction()) then
            -- PullLever
            return 'ToThrow'
        end
        if (InputSameAsDirection()) then
            if (InputRun()) then
                return 'ToRunning'
            elseif (InputSneak()) then
                return 'ToSneak'
            else
                return 'ToWalk'
            end
        end
        if (InputNotSameAsDirection()) then
            FlipDirection()
            return 'StandingTurn'
        end
        if (InputChant()) then return 'ToChant' end
        if (InputHop()) then return 'ToHop' end

        --  StandSpeakXYZ
        -- ToKnockBackStanding
        -- ToFallingNoGround 
     end
}

states.StandingTurn = 
{
    animation = 'AbeStandTurnAround',
    condition = function(s) if s:IsLastFrame() then return 'Stand' end end
}

states.ToWalk = 
{
    animation = 'AbeStandToWalk',
    condition = function(s) if InputLeft() then return 'Walking' end end
}

states.Walking = 
{
    animation = 'AbeWalking',
    condition = function(s) if InputLeft() then return 'ToStand' end end
}

states.ToStand = 
{
    animation = 'AbeWalkToStand',
    condition = function(s) if InputLeft() then return 'Stand' end end
}

states.ToCrouch = 
{
    animation = 'AbeStandToCrouch',
    condition = function(s) if s:IsLastFrame() then return 'Crouch' end end
}

states.Crouch = 
{
    animation = 'AbeCrouchIdle',
    condition = function(s) if true then return 'CrouchToStand' end end
}

states.CrouchToStand = 
{
    animation = 'AbeCrouchToStand',
    condition = function(s) if s:IsLastFrame() then return 'Stand' end end
}

states.Active = states.Stand

function init(self)
    self:SetAnimation(states.Active.animation)
end

function update(self)
    local nextState = states.Active.condition(self)
    if nextState ~= nil then
       print(nextState)
       states.Active = states[nextState]
       self:SetAnimation(states.Active.animation)
    end
end

--update()

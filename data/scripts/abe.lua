local kLeft = false
local kRight = true

-- Composite helpers
function InputSameAsDirection(s)
    return (s:InputLeft() and s:Direction() == kLeft) or (s:InputRight() and s:Direction() == kRight)
end

local function InputNotSameAsDirection(s)
    return (s:InputLeft() and s:Direction() == kRight) or (s:InputRight() and s:Direction() == kLeft)
end

function init(self)
    self.states = {}
    self.states.Stand =
    {
        animation = 'AbeStandIdle',
        condition = function(s)
            if (s:InputDown()) then
                -- ToHoistDown
                return 'ToCrouch'
            end

            if (s:InputUp()) then
                -- ToHoistUp
                return 'ToJump'
            end

            if (s:InputAction()) then
                -- PullLever
                return 'ToThrow'
            end

            if (InputSameAsDirection(s)) then
                if (s:InputRun()) then
                    return 'ToRunning'
                elseif (s:InputSneak()) then
                    return 'ToSneak'
                else
                    return 'ToWalk'
                end
            end

            if (InputNotSameAsDirection(s)) then
                s:FlipDirection()
                return 'StandingTurn'
            end

            if (s:InputChant()) then return 'ToChant' end
            if (s:InputHop()) then return 'ToHop' end

            --  StandSpeakXYZ
            -- ToKnockBackStanding
            -- ToFallingNoGround 
         end
    }
    
    self.states.StandingTurn = 
    {
        animation = 'AbeStandTurnAround',
        condition = function(s) if s:IsLastFrame() then return 'Stand' end end
    }

    self.states.ToWalk = 
    {
        animation = 'AbeStandToWalk',
        condition = function(s) if s:IsLastFrame() then return 'Walking' end end
    }

    self.states.Walking = 
    {
        animation = 'AbeWalking',
        condition = function(s) 
            if (InputSameAsDirection(s) == false) then
                return 'ToStand' 
            end 
        end
    }

    self.states.ToStand = 
    {
        animation = 'AbeWalkToStand',
        condition = function(s) if s:IsLastFrame() then return 'Stand' end end
    }

    self.states.ToCrouch = 
    {
        animation = 'AbeStandToCrouch',
        condition = function(s) if s:IsLastFrame() then return 'Crouch' end end
    }

    self.states.Crouch = 
    {
        animation = 'AbeCrouchIdle',
        condition = function(s) if s:InputUp() then return 'CrouchToStand' end end
    }

    self.states.CrouchToStand = 
    {
        animation = 'AbeCrouchToStand',
        condition = function(s) if s:IsLastFrame() then return 'Stand' end end
    }
    
    self.states.Active = self.states.Stand
    self:SetAnimation(self.states.Active.animation)
end

function update(self)
    local nextState = self.states.Active.condition(self)
    if nextState ~= nil then
       print(nextState)
       self.states.Active = self.states[nextState]
       if self.states.Active == nil then
          print("ERROR: State " .. nextState .. " not found!")
       end
       self:SetAnimation(self.states.Active.animation) 
    end
end

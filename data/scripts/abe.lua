-- Composite helpers
function InputSameAsDirection(s)
    return (s:InputLeft() and s:FacingLeft()) or (s:InputRight() and s:FacingRight())
end

local function InputNotSameAsDirection(s)
    return (s:InputLeft() and s:FacingRight()) or (s:InputRight() and s:FacingLeft())
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
                s:FlipXDirection()
                return 'StandingTurn'
            end

            if (s:InputChant()) then return 'ToChant' end
            if (s:InputHop()) then return 'ToHop' end

            --  StandSpeakXYZ
            -- ToKnockBackStanding
            -- ToFallingNoGround 
         end
    }
    
    self.states.Crouch = 
    {
        animation = 'AbeCrouchIdle',
        condition = function(s) 
            if s:InputUp() then return 'CrouchToStand' end

            if (InputSameAsDirection(s)) then return 'ToRolling' end

            if (InputNotSameAsDirection(s)) then
                s:FlipXDirection()
                return 'CrouchingTurn'
            end
        end
    }
    
    self.states.ToRolling =
    {
        animation = 'AbeCrouchTurnAround',
        -- TODO: This changes instantly IsLastFrame bug?
        condition = function(s) if s:IsLastFrame() then return 'Crouch' end end
    }

    self.states.CrouchingTurn =
    {
        animation = 'AbeCrouchToRoll',
        condition = function(s) if s:IsLastFrame() then return 'Rolling' end end
    }
    
    self.states.Rolling =
    {
        animation = 'AbeRolling',
        condition = function(s) if(InputSameAsDirection(s) == false)  then return 'Crouch' end end
    }

    self.states.Walking =
    {
        animation = 'AbeWalking',
        condition = function(s) if (InputSameAsDirection(s) == false) then return 'ToStand' end end
    }
    
    self.states.ToJump =
    {
        animation = 'AbeStandToJump',
        condition = function(s) if s:IsLastFrame() then return 'Jumping' end end
    }
    
    self.states.Jumping =
    {
        animation = 'AbeJumpUpFalling',
        condition = function(s) if s:IsLastFrame() then return 'ToHitGround' end end
    }
    
    self.states.ToHitGround =
    {
        animation = 'AbeHitGroundToStand',
        condition = function(s) if s:IsLastFrame() then return 'Stand' end end
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

    self.states.CrouchToStand = 
    {
        animation = 'AbeCrouchToStand',
        condition = function(s) if s:IsLastFrame() then return 'Stand' end end
    }
    
    print("Stand")
    self.states.Active = self.states.Stand
    self:SetAnimation(self.states.Active.animation)
end

function update(self)
    local nextState = self.states.Active.condition(self)
    if nextState ~= nil then
       local state = self.states[nextState]
       if state == nil then
          print("ERROR: State " .. nextState .. " not found!")
       else
            print(nextState)
            self.states.Active = state
            self:SetAnimation(self.states.Active.animation)
       end
    end
end

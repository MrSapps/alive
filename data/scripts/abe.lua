-- Composite helpers
function InputSameAsDirection(s, i)
    return (i:InputLeft() and s:FacingLeft()) or (i:InputRight() and s:FacingRight())
end

local function InputNotSameAsDirection(s, i)
    return (i:InputLeft() and s:FacingRight()) or (i:InputRight() and s:FacingLeft())
end

function init(self)
    self.states = {}
    self.states.Stand =
    {
        animation = 'AbeStandIdle',
        condition = function(s, i)
            if (i:InputDown()) then
                -- ToHoistDown
                return 'ToCrouch'
            end

            if (i:InputUp()) then
                -- ToHoistUp
                return 'ToJump'
            end

            if (i:InputAction()) then
                -- PullLever
                return 'ToThrow'
            end

            if (InputSameAsDirection(s, i)) then
                if (i:InputRun()) then
                    return 'ToRunning'
                elseif (i:InputSneak()) then
                    return 'ToSneak'
                else
                    return 'ToWalk'
                end
            end

            if (InputNotSameAsDirection(s, i)) then
                return 'StandingTurn'
            end

            if (i:InputChant()) then return 'ToChant' end
            if (i:InputJump()) then return 'ToHop' end

            --  StandSpeakXYZ
            -- ToKnockBackStanding
            -- ToFallingNoGround 
         end
    }
    
    self.states.Crouch = 
    {
        animation = 'AbeCrouchIdle',
        condition = function(s, i) 
            if i:InputUp() then return 'CrouchToStand' end

            if (InputSameAsDirection(s, i)) then return 'ToRolling' end

            if (InputNotSameAsDirection(s, i)) then
                return 'CrouchingTurn'
            end
        end
    }
    
    self.states.ToRolling =
    {
        animation = 'AbeCrouchToRoll',
        condition = function(s, i) if s:IsLastFrame() then return 'Rolling' end end
    }

    self.states.CrouchingTurn =
    {
        animation = 'AbeCrouchTurnAround',
        condition = function(s, i) 
            if s:IsLastFrame() then 
                s:FlipXDirection()
                return 'Crouch'
            end 
        end
    }
    
    self.states.Rolling =
    {
        animation = 'AbeRolling',
        condition = function(s, i) if(InputSameAsDirection(s, i) == false) then return 'Crouch' end end
    }

    self.states.Walking =
    {
        animation = 'AbeWalking',
        condition = function(s, i) if (InputSameAsDirection(s, i) == false) then return 'ToStand' end end
    }
    
    self.states.ToJump =
    {
        animation = 'AbeStandToJump',
        condition = function(s, i) if s:IsLastFrame() then return 'Jumping' end end
    }
    
    self.states.Jumping =
    {
        animation = 'AbeJumpUpFalling',
        condition = function(s, i) if s:IsLastFrame() then return 'ToHitGround' end end
    }
    
    self.states.ToHitGround =
    {
        animation = 'AbeHitGroundToStand',
        condition = function(s, i) if s:IsLastFrame() then return 'Stand' end end
    }

    self.states.StandingTurn = 
    {
        animation = 'AbeStandTurnAround',
        condition = function(s, i) 
            if s:IsLastFrame() then
                s:FlipXDirection()
                return 'Stand'
            end
        end
    }

    self.states.ToWalk = 
    {
        animation = 'AbeStandToWalk',
        condition = function(s, i) if s:IsLastFrame() then return 'Walking' end end
    }

    self.states.ToStand = 
    {
        animation = 'AbeWalkToStand',
        condition = function(s, i) if s:IsLastFrame() then return 'Stand' end end
    }

    self.states.ToCrouch = 
    {
        animation = 'AbeStandToCrouch',
        condition = function(s, i) if s:IsLastFrame() then return 'Crouch' end end
    }

    self.states.CrouchToStand = 
    {
        animation = 'AbeCrouchToStand',
        condition = function(s, i) if s:IsLastFrame() then return 'Stand' end end
    }

    print("Stand")
    self.states.Active = self.states.Stand
    self:SetAnimation(self.states.Active.animation)
end

function update(self, input)
    local nextState = self.states.Active.condition(self, input)
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

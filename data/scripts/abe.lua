-- Composite helpers
function InputSameAsDirection(s, i)
    return (i:InputLeft() and s:FacingLeft()) or (i:InputRight() and s:FacingRight())
end

local function InputNotSameAsDirection(s, i)
    return (i:InputLeft() and s:FacingRight()) or (i:InputRight() and s:FacingLeft())
end

-- TODO: Grid size isn't constant!
local kGridWidth = 25
local kSneakSpeed = kGridWidth / 10
local kWalkSpeed = kGridWidth / 9
local kRunSpeed = kGridWidth / 4
local kToHopXSpeed = 17
local kHopXSpeed = 13.57
local kHopYSpeed = -2.7

local function MoveX(s, speed)
    if s:FacingRight() then
        s.mXPos = s.mXPos + speed
    else
        s.mXPos = s.mXPos - speed
    end
end

local function MoveY(s, speed)
    s.mYPos = s.mYPos + speed
end

local function Sneak(s) MoveX(s, kSneakSpeed) end
local function Walk(s) MoveX(s, kWalkSpeed) end
local function Run(s) MoveX(s, kRunSpeed) end

function init(self)
    self.states = {}
    self.states.Stand =
    {
        animation = 'AbeStandIdle',
        enter = function(s, i) s:SnapToGrid() end,

        condition = function(s, i)
            if (i:InputDown()) then
                -- ToHoistDown
                return 'ToCrouch'
            end

            if (i:InputJump()) then
                return 'ToHop'
            end

            if (i:InputUp()) then
                -- ToHoistUp, AbeStandToUseStone
                return 'ToJump'
            end
            
            -- TODO: Holding the input down makes IDunno repeat, this isn't what happens in the real game
            if (i:InputThrow()) then
                -- TODO: Check if we have anything to throw
                return 'ToIDunno'
            end

            if (i:InputAction()) then
                return 'PullLever'
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

            if (i:InputChant()) then return 'Chanting' end
            if (i:InputJump()) then return 'ToHop' end

            --  StandSpeakXYZ
            -- ToKnockBackStanding
            -- ToFallingNoGround 
         end
    }
    
    self.states.ToHop =
    {
        animation = 'AbeStandToHop',
        condition = function(s, i) 
            if (s:FrameNumber() == 9) then MoveX(s, kToHopXSpeed) end
            if (s:IsLastFrame()) then return 'Hopping' end 
        end
    }

    self.states.Hopping =
    {
        animation = 'AbeHopping',
        condition = function(s, i)
            MoveX(s, kHopXSpeed)
            if (s:IsLastFrame()) then return 'HopToStand' end 
        end
    }

    self.states.HopToStand =
    {
        animation = 'AbeHoppingToStand',
        condition = function(s, i) 
            if (s:IsLastFrame()) then
                s:SnapToGrid()
                return 'Stand'
            end 
        end
    }

    self.states.PullLever =
    {
        animation = 'AbeStandPullLever',
        condition = function(s, i) if (s:IsLastFrame()) then return 'Stand'end end
    }

    self.states.ToIDunno =
    {
        animation = 'AbeStandToIDunno',
        condition = function(s, i) if (s:IsLastFrame()) then return 'IDunno'end end
    }

    self.states.IDunno =
    {
        animation = 'AbeIDunno',
        condition = function(s, i) if (s:IsLastFrame()) then return 'Stand'end end
    }

    self.states.ToRunning =
    {
        animation = 'AbeStandToRun',
        condition = function(s, i) 
            if (s:IsLastFrame()) then return 'Running'end 
            Run(s)
        end
    }
   
    self.states.Running =
    {
        animation = 'AbeRunning',
        condition = function(s, i) 
            if (InputNotSameAsDirection(s, i)) then
                return 'RuningToSkidTurn'
            end

            if (InputSameAsDirection(s, i)) then
               if (i:InputRun() == false) then
                   return 'RunningToWalking'
               end
               if (i:InputJump()) then
                   return 'RunningToJump'
               end
            else
               return 'ToSkidStop'
            end
            Run(s)
        end
    }
    
    self.states.ToSkidStop =
    {
        animation = 'AbeRunningSkidStop',
        condition = function(s, i) if (s:IsLastFrame()) then return 'Stand'end end
    }
    self.states.RunningToJump =
    {
        animation = 'AbeRuningToJump',
        condition = function(s, i) if (s:IsLastFrame()) then return 'RunningJump'end end
    }

    self.states.RunningJump =
    {
        animation = 'AbeRunningJumpInAir',
        condition = function(s, i) if (s:IsLastFrame()) then return 'RunningJumpFalling'end end
    }

    self.states.RunningJumpFalling =
    {
        animation = 'AbeFallingToLand',
        -- TODO: Skid stop/knock back etc
        condition = function(s, i) if (s:IsLastFrame()) then return 'RunningJumpLandToRunning'end end
    }

    self.states.RunningJumpLandToRunning =
    {
        animation = 'AbeLandToRunning',
        -- TODO: Land to walk etc
        condition = function(s, i) if (s:IsLastFrame()) then return 'Running'end end
    }

    self.states.RunningToWalking =
    {
        animation = 'AbeRunningToWalkingMidGrid',
        condition = function(s, i) 
            if (s:IsLastFrame()) then return 'Walking'end
            Run(s)
        end
    }

    self.states.Sneaking =
    {
        animation = 'AbeSneaking',
        condition = function(s, i) 
            if (InputNotSameAsDirection(s, i)) then
                return 'StandingTurn'
            end

             if (InputSameAsDirection(s, i)) then
                if (i:InputSneak() == false) then
                    -- TODO: Can also be AbeSneakingToWalkingMidGrid
                    return 'SneakingToWalking'
                end
             else
                return 'ToStand'
             end

             Sneak(s)
        end
    }

    self.states.WalkingToRunning =
    {
        animation = 'AbeWalkingToRunning',
        condition = function(s, i) 
            if (s:IsLastFrame()) then return 'Running'end 
            Run(s)
        end
    }

    self.states.WalkingToSneaking =
    {
        animation = 'AbeWalkingToSneaking',
        condition = function(s, i) if (s:IsLastFrame()) then return 'Sneaking'end end
    }

    self.states.SneakingToWalking =
    {
        animation = 'AbeSneakingToWalking',
        condition = function(s, i) if (s:IsLastFrame()) then return 'Walking'end end
    }

    self.states.ToSneak =
    {
        animation = 'AbeStandToSneak',
        condition = function(s, i) 
            if (s:IsLastFrame()) then return 'Sneaking'end 
            Sneak(s)
        end
    }

    self.states.SneakToStand =
    {
        animation = 'AbeSneakToStand',
        condition = function(s, i) 
            if (s:IsLastFrame()) then return 'Stand' end
            Sneak(s)
        end
    }

    self.states.Chanting =
    {
        animation = 'AbeStandToChant',
        condition = function(s, i) if (i:InputChant() == false) then return 'AbeChantToStand' end end
    }

    self.states.AbeChantToStand =
    {
        animation = 'AbeChantToStand',
        condition = function(s, i)  if (s:IsLastFrame()) then return 'Stand' end end
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
        condition = function(s, i) 
            if (InputSameAsDirection(s, i) == false) then 
                return 'ToStand' 
            end
            if (i:InputRun()) then
                return 'WalkingToRunning'
            elseif (i:InputSneak()) then
                return 'WalkingToSneaking'
            end
            Walk(s)
        end
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

    self.states.RuningToSkidTurn = 
    {
        animation = 'AbeRunningToSkidTurn',
        condition = function(s, i) if s:IsLastFrame() then return 'RuningTurn' end end
    }
    
    self.states.RuningTurn = 
    {
        animation = 'AbeRunningTurnAround',
        condition = function(s, i) 
            if s:IsLastFrame() then
                s:FlipXDirection()
                return 'Running'
            end
        end
    }
    self.states.StandingTurn = 
    {
        animation = 'AbeStandTurnAround',
        condition = function(s, i) 
            if s:IsLastFrame() then
                s:FlipXDirection()
                -- TODO: AbeRunningTurnAround if running in turning direction
                return 'Stand'
            end
        end
    }

    self.states.ToWalk = 
    {
        animation = 'AbeStandToWalk',
        condition = function(s, i)
            Walk(s)
            if s:IsLastFrame() then return 'Walking' end 
        end
    }

    self.states.ToStand = 
    {
        animation = 'AbeWalkToStand',
        condition = function(s, i) 
            if s:IsLastFrame() then return 'Stand' end 
            Walk(s)
        end
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

    self:ScriptLoadAnimations()

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
            if (self.states.Active.enter ~= nil) then
                self.states.Active.enter(self, input)
            end
       end
    end
end

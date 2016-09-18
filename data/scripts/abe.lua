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
    self.states.name = "abe"

    self.states.SayHelloPart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SayHelloPart2' end end
    }
    self.states.SayHelloPart2 =
    {
        animation = 'AbeStandSpeak2',
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_HELLO") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }
    
    self.states.SayFollowMePart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SayFollowMePart2' end end
    }
    self.states.SayFollowMePart2 =
    {
        animation = 'AbeStandSpeak3',
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_FOLLOWME") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }
    
    self.states.SayWaitPart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SayWaitPart2' end end
    }
    self.states.SayWaitPart2 =
    {
        animation = 'AbeStandingSpeak4',
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_WAIT") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }

    self.states.SayAngerPart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SayAngerPart2' end end
    }
    self.states.SayAngerPart2 =
    {
        animation = 'AbeStandingSpeak4',
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_ANGRY") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }
    
    self.states.SayWorkPart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SayWorkPart2' end end
    }
    self.states.SayWorkPart2 =
    {
        animation = 'AbeStandingSpeak4',
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_WORK") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }

    self.states.SayAllYaPart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SayAllYaPart2' end end
    }
    self.states.SayAllYaPart2 =
    {
        animation = 'AbeStandSpeak2',
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_ALLYA") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }

    self.states.SaySorryPart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SaySorryPart2' end end
    }
    self.states.SaySorryPart2 =
    {
        animation = 'AbeStandSpeak5',
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_SORRY") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }

    self.states.SayStopItPart1 =
    {
        animation = 'AbeStandSpeak1',
        tick = function(s, i) if (s:IsLastFrame()) then return 'SayStopItPart2' end end
    }
    self.states.SayStopItPart2 =
    {
        animation = 'AbeStandSpeak3',
        -- TODO: StopIt is actually a SEQ
        enter = function(s, i) PlaySoundEffect("GAMESPEAK_MUD_NO_SAD") end,
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand' end end
    }

    self.states.Stand =
    {
        animation = 'AbeStandIdle',
        --enter = function(s, i) s:SnapToGrid() end,

        tick = function(s, i)
            if (i:InputGameSpeak1()) then
                return 'SayHelloPart1'
            end
            if (i:InputGameSpeak2()) then
                return 'SayFollowMePart1'
            end
            if (i:InputGameSpeak3()) then
                return 'SayWaitPart1'
            end
            if (i:InputGameSpeak4()) then
                return 'SayWorkPart1'
            end
            if (i:InputGameSpeak5()) then
                return 'SayAngerPart1'
            end
            if (i:InputGameSpeak6()) then
                return 'SayAllYaPart1'
            end
            if (i:InputGameSpeak7()) then
                return 'SaySorryPart1'
            end
            if (i:InputGameSpeak8()) then
                return 'SayStopItPart1'
            end

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
                local xpos = self.mXPos
                if self:FacingLeft() then
                    xpos = xpos - (kGridWidth + 5)
                else
                    xpos = xpos + (kGridWidth + 5)
                end

                local lever = GetMapObject(xpos, self.mYPos, "lever")
                if (lever == nil) then
                    print("No lever at " .. xpos .. "," .. self.mYPos)
                else
                    if (lever.states.CanBeActivated()) then
                        lever.states.Activate(self:FacingLeft())
                        return 'PullLever'
                    else
                        print("Lever is not in right state")
                    end
                end
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
        tick = function(s, i) 
            if (s:FrameNumber() == 9) then MoveX(s, kToHopXSpeed) end
            if (s:IsLastFrame()) then return 'Hopping' end 
        end
    }

    self.states.Hopping =
    {
        animation = 'AbeHopping',
        tick = function(s, i)
            MoveX(s, kHopXSpeed)
            if (s:IsLastFrame()) then return 'HopToStand' end 
        end
    }

    self.states.HopToStand =
    {
        animation = 'AbeHoppingToStand',
        tick = function(s, i) 
            if (s:IsLastFrame()) then
                s:SnapToGrid()
                return 'Stand'
            end 
        end
    }

    self.states.PullLever =
    {
        animation = 'AbeStandPullLever',
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand'end end
    }

    self.states.ToIDunno =
    {
        animation = 'AbeStandToIDunno',
        tick = function(s, i) if (s:IsLastFrame()) then return 'IDunno'end end
    }

    self.states.IDunno =
    {
        animation = 'AbeIDunno',
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand'end end
    }

    self.states.ToRunning =
    {
        animation = 'AbeStandToRun',
        tick = function(s, i) 
            if (s:IsLastFrame()) then return 'Running'end 
            Run(s)
        end
    }
   
    self.states.Running =
    {
        animation = 'AbeRunning',
        tick = function(s, i) 
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
        tick = function(s, i) if (s:IsLastFrame()) then return 'Stand'end end
    }
    self.states.RunningToJump =
    {
        animation = 'AbeRuningToJump',
        tick = function(s, i) if (s:IsLastFrame()) then return 'RunningJump'end end
    }

    self.states.RunningJump =
    {
        animation = 'AbeRunningJumpInAir',
        tick = function(s, i) if (s:IsLastFrame()) then return 'RunningJumpFalling'end end
    }

    self.states.RunningJumpFalling =
    {
        animation = 'AbeFallingToLand',
        -- TODO: Skid stop/knock back etc
        tick = function(s, i) if (s:IsLastFrame()) then return 'RunningJumpLandToRunning'end end
    }

    self.states.RunningJumpLandToRunning =
    {
        animation = 'AbeLandToRunning',
        -- TODO: Land to walk etc
        tick = function(s, i) if (s:IsLastFrame()) then return 'Running'end end
    }

    self.states.RunningToWalking =
    {
        animation = 'AbeRunningToWalkingMidGrid',
        tick = function(s, i) 
            if (s:IsLastFrame()) then return 'Walking'end
            Run(s)
        end
    }

    self.states.Sneaking =
    {
        animation = 'AbeSneaking',
        tick = function(s, i) 
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
        tick = function(s, i) 
            if (s:IsLastFrame()) then return 'Running'end 
            Run(s)
        end
    }

    self.states.WalkingToSneaking =
    {
        animation = 'AbeWalkingToSneaking',
        tick = function(s, i) if (s:IsLastFrame()) then return 'Sneaking'end end
    }

    self.states.SneakingToWalking =
    {
        animation = 'AbeSneakingToWalking',
        tick = function(s, i) if (s:IsLastFrame()) then return 'Walking'end end
    }

    self.states.ToSneak =
    {
        animation = 'AbeStandToSneak',
        tick = function(s, i) 
            if (s:IsLastFrame()) then return 'Sneaking'end 
            Sneak(s)
        end
    }

    self.states.SneakToStand =
    {
        animation = 'AbeSneakToStand',
        tick = function(s, i) 
            if (s:IsLastFrame()) then return 'Stand' end
            Sneak(s)
        end
    }

    self.states.Chanting =
    {
        animation = 'AbeStandToChant',
        tick = function(s, i) if (i:InputChant() == false) then return 'AbeChantToStand' end end
    }

    self.states.AbeChantToStand =
    {
        animation = 'AbeChantToStand',
        tick = function(s, i)  if (s:IsLastFrame()) then return 'Stand' end end
    }
    
    self.states.Crouch = 
    {
        animation = 'AbeCrouchIdle',
        tick = function(s, i) 
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
        tick = function(s, i)
            MoveX(s, kRunSpeed)
            if s:IsLastFrame() then return 'Rolling' end 
        end
    }

    self.states.CrouchingTurn =
    {
        animation = 'AbeCrouchTurnAround',
        tick = function(s, i) 
            if s:IsLastFrame() then 
                s:FlipXDirection()
                return 'Crouch'
            end 
        end
    }
    
    self.states.Rolling =
    {
        animation = 'AbeRolling',
        tick = function(s, i) 
            if(InputSameAsDirection(s, i) == false) then
                return 'Crouch'
            else
                MoveX(s, kRunSpeed)
            end 
        end
    }

    self.states.Walking =
    {
        animation = 'AbeWalking',
        tick = function(s, i)
            Walk(s)
            local frame = s:FrameNumber()
            if (frame == 2) then
                PlaySoundEffect("MOVEMENT_MUD_STEP")
                if (InputSameAsDirection(s, i) == false) then
                    return 'ToStand'
                elseif (i:InputRun()) then
                    return 'WalkingToRunning'
                elseif (i:InputSneak()) then
                    return 'WalkingToSneaking'
                end
            end
        end
    }
    
    self.states.ToJump =
    {
        animation = 'AbeStandToJump',
        tick = function(s, i) if s:IsLastFrame() then return 'Jumping' end end
    }
    
    self.states.Jumping =
    {
        animation = 'AbeJumpUpFalling',
        tick = function(s, i) if s:IsLastFrame() then return 'ToHitGround' end end
    }
    
    self.states.ToHitGround =
    {
        animation = 'AbeHitGroundToStand',
        tick = function(s, i) if s:IsLastFrame() then return 'Stand' end end
    }

    self.states.RuningToSkidTurn = 
    {
        animation = 'AbeRunningToSkidTurn',
        tick = function(s, i) if s:IsLastFrame() then return 'RuningTurn' end end
    }
    
    self.states.RuningTurn = 
    {
        animation = 'AbeRunningTurnAround',
        tick = function(s, i) 
            if s:IsLastFrame() then
                s:FlipXDirection()
                return 'Running'
            end
        end
    }
    self.states.StandingTurn = 
    {
        animation = 'AbeStandTurnAround',
        tick = function(s, i) 
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
        enter = function(s,i) Walk(s) end,
        tick = function(s, i)
            Walk(s)
            if s:IsLastFrame() then return 'Walking' end
        end
    }
    
    self.states.ToStand = 
    {
        animation = 'AbeWalkToStand',
        tick = function(s, i)
            if s:IsLastFrame() then return 'Stand' end
            Walk(s)
        end
    }

    self.states.ToCrouch = 
    {
        animation = 'AbeStandToCrouch',
        tick = function(s, i) if s:IsLastFrame() then return 'Crouch' end end
    }

    self.states.CrouchToStand = 
    {
        animation = 'AbeCrouchToStand',
        tick = function(s, i) if s:IsLastFrame() then return 'Stand' end end
    }

    self:ScriptLoadAnimations()

    self.states.Active = self.states.Stand
    self:SetAnimation(self.states.Active.animation)
end

function update(self, input)
    local nextState = self.states.Active.tick(self, input)
    if nextState ~= nil then
       local state = self.states[nextState]
       if state == nil then
          print("ERROR: State " .. nextState .. " not found!")
       else
            self.states.Active = state
            print("State: " .. nextState .. " animation: " .. self.states.Active.animation)
            self:SetAnimation(self.states.Active.animation)
            if (self.states.Active.enter ~= nil) then
                self.states.Active.enter(self, input)
            end
       end
    end
end

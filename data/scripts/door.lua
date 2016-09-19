function init_with_data(self, rect, stream)
    -- SHDOOR.BAN_2012_AePc_0
    self.mXPos = rect.x + 11
    self.mYPos = rect.y + rect.h - 19
    
    -- BADOOR.BAN_2012_AePc_0
    self.mXPos = rect.x + 13
    self.mYPos = rect.y + rect.h -2

    self.states = {}
    self.states.name = "door"

    local level = stream:ReadU16()
    local path = stream:ReadU16()
    local camera = stream:ReadU16()
    local scale = stream:ReadU16()
    if (scale == 1) then
        print("WARNING: Half scale not supported")
    end
    local doorNumber = stream:ReadU16()
    self.states.id = stream:ReadU16()
    local targetDoorNumber = stream:ReadU16()
    local skin = stream:ReadU16()
    local startOpen = stream:ReadU16()

    local hubId1 = stream:ReadU16()
    local hubId2 = stream:ReadU16()
    local hubId3 = stream:ReadU16()
    local hubId4 = stream:ReadU16()
    local hubId5 = stream:ReadU16()
    local hubId6 = stream:ReadU16()
    local hubId7 = stream:ReadU16()
    local hubId8 = stream:ReadU16()

    local wipeEffect = stream:ReadU16()
    local xoffset = stream:ReadU16()
    local yoffset = stream:ReadU16()

    local wipeXStart = stream:ReadU16()
    local wipeYStart = stream:ReadU16()

    local abeFaceLeft = stream:ReadU16()
    local closeAfterUse = stream:ReadU16()

    local removeThrowables = stream:ReadU16()
    
    self.mXPos = rect.x + xoffset
    self.mYPos = rect.y + rect.h + yoffset


    self.states.Closed =
    {
        -- AE door types
        animation = "DoorClosed_Barracks", -- DoorToClose_Barracks have to play this in reverse to get DoorToOpen_Barracks
        --animation = "DOOR.BAN_2012_AePc_0", -- mines
        --animation = "SHDOOR.BAN_2012_AePc_0", -- wooden mesh door
        --animation = "TRAINDOR.BAN_2013_AePc_0", -- feeco train door - not aligned!
        --animation = "BRDOOR.BAN_2012_AePc_0", -- brewer door, not aligned
        --animation = "BWDOOR.BAN_2012_AePc_0", -- bone werkz door
        --animation = "STDOOR.BAN_8001_AePc_0", -- blank? 
        --animation = "FDDOOR.BAN_2012_AePc_0", -- feeco normal door
        --animation = "SVZDOOR.BAN_2012_AePc_0", -- scrab vault door

        tick = function(s, i)
            -- TODO: Detect if activated and switch to opening
        end
    }

    self.states.Closing =
    {
        animation = "DoorToClose_Barracks",
        tick = function(s, i) 
            if (s:IsLastFrame()) then 
                --return 'Opening' 
            end
        end
    }
    
    self.states.Opening =
    {
        animation = "DoorToClose_Barracks",
        tick = function(s, i) 
            if (s:IsLastFrame()) then
                --PlaySoundEffect("FX_DOOR")
                --return 'Closing'
            end
        end
    }
    
    if startOpen == 0 then
        self.states.Active = self.states.Closed
    else
        self.states.Active = self.states.Opening
    end

    self:ScriptLoadAnimations()

    self:SetAnimation(self.states.Active.animation)
end

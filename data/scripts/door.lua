function init_with_data(self, rect, stream)
    -- SHDOOR.BAN_2012_AePc_0
    self.mXPos = rect.x + 11
    self.mYPos = rect.y + rect.h - 19
    
    -- BADOOR.BAN_2012_AePc_0
    self.mXPos = rect.x + 13
    self.mYPos = rect.y + rect.h -2

    self.states = {}
    self.states.name = "door"

    self.states.Closed =
    {
        -- AE door types
        animation = "BADOOR.BAN_2012_AePc_0", -- OpenToClose slig huts
        --animation = "DOOR.BAN_2012_AePc_0", -- mines
        --animation = "SHDOOR.BAN_2012_AePc_0", -- wooden mesh door
        --animation = "TRAINDOR.BAN_2013_AePc_0", -- feeco train door - not aligned!
        --animation = "BRDOOR.BAN_2012_AePc_0", -- brewer door, not aligned
        --animation = "BWDOOR.BAN_2012_AePc_0", -- bone werkz door
        --animation = "STDOOR.BAN_8001_AePc_0", -- blank? 
        --animation = "FDDOOR.BAN_2012_AePc_0", -- feeco normal door
        --animation = "SVZDOOR.BAN_2012_AePc_0", -- scrab vault door

        tick = function(s, i)
            -- TODO: Detect if activated switch to opening
        end
    }

    self:ScriptLoadAnimations()

    self.states.Active = self.states.Closed
    self:SetAnimation(self.states.Active.animation)
end

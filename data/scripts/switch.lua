function init_with_data(self, rect, stream)
    self.mXPos = rect.x + 37
    self.mYPos = rect.y + rect.h - 5
    self.states = {}
    self.states.WaitForActivate =
    {
        animation = "SwitchIdle",
        -- Do nothing, other objects will query CanBeActivated and call Activate
        tick = function(s, i) end
    }
    self.states.DoActivateLeft =
    {
        animation = "SwitchActivateLeft",
        tick = function(s, i) if (s:IsLastFrame()) then return 'DoDeactivateLeft' end end
    }
    self.states.DoDeactivateLeft =
    {
        animation = "SwitchDeactivateLeft",
        tick = function(s, i) if (s:IsLastFrame()) then return 'WaitForActivate' end end
    }
    self.states.DoActivateRight =
    {
        animation = "SwitchActivateRight",
        tick = function(s, i) if (s:IsLastFrame()) then return 'DoDeactivateRight' end end
    }
    self.states.DoDeactivateRight =
    {
        animation = "SwitchDeactivateRight",
        tick = function(s, i) if (s:IsLastFrame()) then return 'WaitForActivate' end end
    }
    self.states.CanBeActivated = function()
        print("CanBeActivated")
        -- TODO: Only allow activate in certain states
        --return self.states.Activate == self.states.WaitForActivate
        return true
    end
    self.states.Activate = function(facingLeft)
        print("Activate")
        -- TODO: Activate whatever we are linked to
        --GetObjectWithId(self.states.mId).Activate()
        if facingLeft then
            self.states.Active = self.states.DoActivateRight
        else
            self.states.Active = self.states.DoActivateLeft
        end

        PlaySoundEffect("FX_LEVER")

        self:SetAnimation(self.states.Active.animation)
    end

    self.states.name = "lever"
    self:ScriptLoadAnimations()
    self.states.Active = self.states.WaitForActivate
    self:SetAnimation(self.states.Active.animation)
end
function init_with_data(self, stream)

    self.states = {}
    

    self.states.WaitForActivate =
    {
        animation = "SWITCH1.BAN_1009_AePc_0",
        tick = function(s, i)
            -- TODO: Detect if activated and from which direction
        end
    }

    self:ScriptLoadAnimations()

    self.states.Active = self.states.WaitForActivate
    self:SetAnimation(self.states.Active.animation)
end

function init_with_data(self, rect, stream)
    self.mXPos = rect.x + 10
    self.mYPos = rect.y + 22

    self.states = {}
    self.states.name = "mine"

    stream:ReadU32() -- Skip unused "num patterns"
    stream:ReadU32() -- Skip unused "patterns"
    local scale = stream:ReadU32()

    if (animId == 1) then
        print("Warning: Mine background scale not implemented")
    end

    self.states.WaitingForCollision =
    {
        animation = "LANDMINE.BAN_1036_AePc_0",
        tick = function(s, i) end
    }

    self:ScriptLoadAnimations()

    self.states.Active = self.states.WaitingForCollision
    self:SetAnimation(self.states.Active.animation)
end

function init_with_data(self, stream)

    self.states = {}
    

    local animId = stream:ReadU32()

    if (animId == 1201) then
        animName = "BAP01C06.CAM_1201_AePc_0"
    elseif (animId == 1202) then
        animName = "FARTFAN.BAN_1202_AePc_0"
    else
        animName = "AbeStandSpeak1"
    end

    print("Id is " .. animId)

    self.states.Run =
    {
        animation = animName,
        tick = function(s, i) end
    }

    self:ScriptLoadAnimations()

    self.states.Active = self.states.Run
    self:SetAnimation(self.states.Active.animation)
end

function init_with_data(self, rect, stream)
    self.mXPos = rect.x
    self.mYPos = rect.y

    self.states = {}
    self.states.name = "electric_wall"

    local scale = stream:ReadU16()
    if (scale == 1) then
        print("Warning: electric_wall scale not implemented")
    end

    self.states.id = stream:ReadU16()
    self.states.start_on = stream:ReadU16()
   
    print("WALL ID IS " .. self.states.id .. " START ON IS " .. self.states.start_on .. " SCALE IS " .. scale)

    self.states.On =
    {
        animation = "ELECWALL.BAN_6000_AePc_0",
        tick = function(s, i)
            if self.states.Change then
                self.states.Change = false
                return 'Off'
            end
        end
    }

    self.states.Off =
    {
        animation = "",
        tick = function(s, i)
            if self.states.Change then
                self.states.Change = false
                return 'On'
            end
        end
    }

    self.states.Activate = function(facingLeft)
        print("activate")
        self.states.Change = true
    end

    self:ScriptLoadAnimations()

    if self.states.start_on then
        self.states.Active = self.states.On
    else
        self.states.Active = self.states.Off
    end

    self:SetAnimation(self.states.Active.animation)
end

function Init()
   print("Hello")

   h_echo("hello",
          42,
          true,
          {},
          nil,
          function () return {}	end,
          "done")
 
   SomethingElse()
 
   -- find out what values get returned	from h_echo
   h_echo(h_echo())
 
   -- make the error handler work
   Fail()
end

function SomethingElse()
   -- if you use a package, this is where lua looks
   h_echo(package.path)
end

function Fail()
   failure = nonexistent_variable.indexing.error
end

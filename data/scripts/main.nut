local test = 7;
local a = Actions();
local b = ObjRect();

function init()
{
    log_info("This is a test " + test);
    test = test + 1;

    Actions.Left(5);
    a.IsPressed = true;
    log_info("value is " + a.IsPressed);


    b.x = 7;
    log_info(b.x);
}

function update()
{

}

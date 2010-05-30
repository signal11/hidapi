To run hidtest.exe from this folder, make sure to copy hidapi.dll from
hidapi\objfre_wxp_x86\i386 to this folder. Note that it's probably better
to just run it from inside the visual studio project.

To run it from Visual Studio, make sure that hidapi.dll is _not_ in this directory (as it will override the one in ..\ that the hidtest project will copy automatically).
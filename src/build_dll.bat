@echo off

pushd ..\build
del *.pdb
cl %1 -ohotload_trigger.dll   -DEBUG -Od -Zi -LD /link /EXPORT:OnModuleLoad /EXPORT:GameStartup /EXPORT:GameUpdate /EXPORT:GameRender -PDB:hotload_%random%.pdb
popd
 
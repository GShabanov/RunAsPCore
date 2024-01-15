# Run As PCore - Setting threads for Denuvo

## Description
Due to a bug within Denuvo protection, it does not correctly recognize processor cores. 
It is almost impossible to resist this, except to get rid of the defense. 
However, there is a method: you need to eliminate low-performing ECores.  In this case, these threads 
will not be scheduled and the protection will not experience problems.

## Run
As the first parameter, the program takes the name of the application that needs to be launched.

### RunAsPCore.exe DOOMEternalx64vk.exe

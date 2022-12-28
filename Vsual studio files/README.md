Here are the steps to get Visual studio project running 

1. Download and put MAX/SDK into MAX packages filder. 
	I used latest verision. 

2. take from folder "visual studio files" folder binaural_ns~ and put in in the MAX/SDK sourse files directory. for me it was - ..\Max 8\Packages\max-sdk-main\source\advanced\

3. Open visual studio Visual studio command prompt:

	cd C:\Users\username\Documents\Max 8\Packages\max-sdk-main\build
	cmake -G "Visual Studio 17 2022" ..   ( or whatever VS you are using)

	this will (re-)make all MAX/SDK projects. 

4. unzip "oscpack_1_1_0.zip" and place folder somewhere. 
	I used ..\Max 8\Packages\max-sdk-main\source\oscpack_1_1_0. 	(But i think this doen't matter.)

5. make same procedure with c-make. Open visual studio Visual studio command prompt:

	cd C:\Users\username\Documents\Max 8\Packages\max-sdk-main\source\oscpack_1_1_0
	cmake -G "Visual Studio 17 2022" ..   ( or whatever VS you are using)

	this will make oscpack project

6. Go ahead and build all in oscpack project.

7. Go to MAX/SDK folder to directory build and open max-sdk-main.sln. Or go to "max-sdk-main\build\source\advancedbinaural_ns~" and open solution file "binaural_ns~.sln"

8. Add includes and libraries. Right click on solution explorer on the ml_tf_inferencer and go to propoerties.

	go to C/C++ -> general -> additional incude directories =  and add "C:\Users\username\Documents\Max 8\Packages\max-sdk-main\source\oscpack_1_1_0"
	go to linker -> input -> additional dependencies = Debug\oscpack.lib	(this may need specifyication of the exact path.)
						Ws2_32.lib
						winmm.lib 

9. ml_tf_inferencer.sln must be ready to run and compile MAX extention.

P.S. I had a problem 
	in both oscpack_1_1_0 and binaural_ns~ the setting of - C/C++ -> Code Generation -> runtime libarary = must be the same to run.
	I had "Multi-threaded Debug (/MTd)" but I think it shouldn't be a problem as far as it's consistent and same in both. 





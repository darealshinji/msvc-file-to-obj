CL       = cl -nologo
CPPFLAGS = -DWIN32_LEAN_AND_MEAN -DUTF8_EVERYWHERE
CFLAGS   = -W4 -O2 -I.
CXXFLAGS = -W4 -O2 -I. -EHsc -std:c++20


all: file_to_obj.exe test.exe

clean:
	-del /q *.exe *.obj

file_to_obj.exe: file_to_obj.cpp main.cpp
	$(CL) $(CXXFLAGS) $(CPPFLAGS) $** -Fe$@ -link -manifest:embed -manifestinput:file_to_obj.manifest

data.obj: file_to_obj.exe
	file_to_obj.exe $@ Lorem1.txt Lorem2.txt Lorem3.txt

test.exe: test.c data.obj
	$(CL) $(CFLAGS) $(CPPFLAGS) $** -Fe$@


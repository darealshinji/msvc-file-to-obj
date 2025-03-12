CL       = cl -nologo
CPPFLAGS = -DWIN32_LEAN_AND_MEAN
CFLAGS   = -W4 -O2 -I.


all: file_to_obj.exe test.exe

clean:
	-del /q *.exe *.obj

file_to_obj.exe: file_to_obj.c simple_basename.c main.c
	$(CL) $(CFLAGS) $(CPPFLAGS) $** -Fe$@

lorem.txt.obj: file_to_obj.exe
	file_to_obj.exe lorem.txt

test.exe: test.c lorem.txt.obj
	$(CL) $(CFLAGS) $(CPPFLAGS) $** -Fe$@


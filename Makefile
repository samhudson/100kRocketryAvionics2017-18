default:
	gcc ./data_generator/datagen.c -std=c99 -lm -o ./gen -I./conversion/
	gcc logging/logger.c  -std=c99 -lm -o logging/logger -Wno-format-security
wgcc:
	gcc logging/logger.c -std=c99 -lm -o logging/logger
wclang:
	clang logging/logger.c -std=c99 -lm -o logging/logger
test:
	gcc ./data_generator/datagen.c -std=c99 -lm -o ./gen
	gcc ./logging/logger.c  -std=c99 -lm -o ./logger -Wno-format-security
	gcc ./logging/logtest1.c -std=c99 -lm -o ./test1
	rm -f ./testlog.txt
	./gen -test -to testlog.txt
	./gen -test -pipe ./logger 
	./test1
	g++ -o ./3d_trace/trace_test1  ./3d_trace/trace_test1.cpp  -lGL  -lGLU  /usr/lib/x86_64-linux-gnu/libglut.so  -lm -I./3d_trace/include -Wno-write-strings
	rm ./logger ./test1 ./log.txt ./testlog.txt ./gen ./3d_trace/trace_test1
test2:
	g++ -o ./3d_trace/trace_test1  ./3d_trace/trace_test1.cpp  -lGL  -lGLU  ./libglut.so  -lm -I./3d_trace/include -Wno-write-strings
	./3d_trace/trace_test1
clean: 
	rm ./logger ./test1 ./log.txt ./testlog.txt ./gen ./3d_trace/trace_test1

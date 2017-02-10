all: sample2D

sample2D: Sample_GL3_2D.cpp glad.c
	g++ -std=c++11 -o sample2D Sample_GL3_2D.cpp glad.c -lpthread -lao -lmpg123 -lGL -lglfw -ldl

clean:
	rm sample2D

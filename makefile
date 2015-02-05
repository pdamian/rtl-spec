# MAKEFILE
# target: dependencies
# [tab] system command
#
# $@ : target
# $^ : all dependencies
# $< : first dependency

SRC_PATH = src/
INC_PATH = include/
OBJ_PATH = build/
DAT_PATH = dat/
CC = gcc
override CFLAGS += -c -Wall
INCL = -I $(INC_PATH)

LDFLAGS_CPU = -lpthread -lz -lrt -lm `pkg-config --cflags --libs librtlsdr` -lfftw
LDFLAGS_GPU = -lpthread -lz -lrt -lm `pkg-config --cflags --libs librtlsdr`
LDFLAGS_COL = -lpthread -lz -lrt

EXE_CPU = sensor_cpu.exe
EXE_GPU = sensor_gpu.exe
EXE_COL = collector.exe

MKDIR_P = mkdir -p

SRC_CPU = src/sensor/Sensor.c src/UTI.c src/ITE.c src/QUE.c src/TCP.c src/THR.c src/SDR.c src/FFT.c
SRC_GPU = $(wildcard $(SRC_PATH)*.c $(SRC_PATH)sensor/*.c)
SRC_COL = src/collector/Collector.c src/ITE.c src/QUE.c src/TCP.c src/THR.c

OBJ_CPU = $(subst src/, $(OBJ_PATH), $(SRC_CPU:.c=.o))
OBJ_CPU_FFT = $(OBJ_CPU:FFT.o=FFT_CPU.o)
OBJ_GPU = $(subst src/, $(OBJ_PATH), $(SRC_GPU:.c=.o))
OBJ_GPU_FFT = $(OBJ_GPU:FFT.o=FFT_GPU.o)
OBJ_COL = $(subst src/, $(OBJ_PATH), $(SRC_COL:.c=.o))

.PHONY sensor_cpu: directories $(SRC_CPU) $(EXE_CPU)

.PHONY sensor_gpu: directories $(SRC_GPU) $(EXE_GPU)

.PHONY collector: directories $(SRC_COL) $(EXE_COL)

.PHONY directories: $(OBJ_PATH) $(DAT_PATH)

$(OBJ_PATH):
	$(MKDIR_P) $(OBJ_PATH)sensor/ $(OBJ_PATH)collector
	
$(DAT_PATH):
	$(MKDIR_P) $(DAT_PATH)/stats

$(EXE_CPU): $(OBJ_CPU_FFT)
	$(CC) $(OBJ_CPU_FFT) -o $@ $(LDFLAGS_CPU)
	
$(EXE_GPU): $(OBJ_GPU_FFT)
	$(CC) $(OBJ_GPU_FFT) -o $@ $(LDFLAGS_GPU)

$(EXE_COL): $(OBJ_COL)
	$(CC) $(OBJ_COL) -o $@ $(LDFLAGS_COL)

build/FFT_CPU.o: src/FFT.c
	$(CC) $(CFLAGS) -DRPI_CPU $(INCL) -o $@ $<
	
build/FFT_GPU.o: src/FFT.c
	$(CC) $(CFLAGS) -DRPI_GPU $(INCL) -o $@ $<

build/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCL) -o $@ $<
	
.PHONY clean:
	rm -rf $(EXE_CPU) $(EXE_GPU) $(EXE_COL) $(OBJ_CPU_FFT) $(OBJ_GPU_FFT) $(OBJ_COL) $(OBJ_PATH)



LDLIBS += -ldl
CFLAGS += -O3
CXXFLAGS += -O3

MWLApplier: gen_rom.o main.o mwllib.o utils.o asardll.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

all: MWLApplier

clean:
	rm *.o
	rm MWLApplier


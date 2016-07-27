CXXFLAGS += -I./
CXXFLAGS += -std=c++11 -Wall -g -c -o

LIB_FILES := -levent -lglog -lgflags -L/usr/local/lib -lgtest -lgtest_main -lpthread

CPP_SOURCES := \
	./threads/thread_manager.cc \
	./threads/simple_thread_factory.cc \
	./threads/mutex.cc \
	./threads/monitor.cc \
	./threads/timer_manager.cc \
	./base/time_util.cc \


CPP_OBJECTS := $(CPP_SOURCES:.cc=.o)


TESTS := \
	./threads/simple_thread_factory_unittest \
	./threads/thread_manager_unittest \
	./splitter_unittest \
	./observer_unittest \

all: $(CPP_OBJECTS) $(TESTS)
.cc.o:
	$(CXX) $(CXXFLAGS) $@ $<

./threads/simple_thread_factory_unittest: ./threads/simple_thread_factory_unittest.o
	$(CXX) -o $@ $< $(CPP_OBJECTS) $(LIB_FILES)
./threads/simple_thread_factory_unittest.o: ./threads/simple_thread_factory_unittest.cc
	$(CXX) -Wno-unused-variable $(CXXFLAGS) $@ $<

./threads/thread_manager_unittest: ./threads/thread_manager_unittest.o
	$(CXX) -o $@ $< $(CPP_OBJECTS) $(LIB_FILES)
./threads/thread_manager_unittest.o: ./threads/thread_manager_unittest.cc
	$(CXX) -Wno-unused-variable $(CXXFLAGS) $@ $<

./splitter_unittest: ./splitter_unittest.o
	$(CXX) -o $@ $< $(CPP_OBJECTS) $(LIB_FILES) third_party/epub_spliter/libepub_spliter.a \
		-lpthread -lz -lmxml -lgumbo -lssl -lpthread
./splitter_unittest.o: ./splitter_unittest.cc
	$(CXX) -Wno-unused-variable $(CXXFLAGS) $@ $<

./observer_unittest: ./observer_unittest.o
	$(CXX) -o $@ $< $(CPP_OBJECTS) $(LIB_FILES)
./observer_unittest.o: ./observer_unittest.cc
	$(CXX) -Wno-unused-variable $(CXXFLAGS) $@ $<

clean:
	rm -fr base/*.o
	rm -fr threads/*.o
	rm -fr *.o
	rm -fr ./test_data/*epub_*epub
	rm -fr $(TESTS)

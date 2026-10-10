# Run from v3d_main/v3d after a normal build:
# make -f Makefile -f ../../tests/object_yflip_gui_test.mk yflip-gui-test
# (Correct relative path from v3d_main/v3d is ../../tests.)
.PHONY: yflip-gui-test
yflip-gui-test: /tmp/vaa3d-object-yflip-gui
/tmp/vaa3d-object-yflip-gui.o: ../../tests/object_yflip_gui_test.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-object-yflip-gui: /tmp/vaa3d-object-yflip-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

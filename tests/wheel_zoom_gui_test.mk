# Run from v3d_main/v3d after a normal build:
# make -f Makefile -f ../../tests/wheel_zoom_gui_test.mk wheel-zoom-gui-test
# (Correct relative path from v3d_main/v3d is ../../tests.)
.PHONY: wheel-zoom-gui-test
wheel-zoom-gui-test: /tmp/vaa3d-wheel-zoom-gui
/tmp/vaa3d-wheel-zoom-gui.o: ../../tests/wheel_zoom_gui_test.cpp ../3drenderer/v3dr_glwidget.h ../3drenderer/WheelStepAccumulator.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-wheel-zoom-gui: /tmp/vaa3d-wheel-zoom-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

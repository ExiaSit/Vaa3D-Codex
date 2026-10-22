.PHONY: swc-color-gui-test
swc-color-gui-test: /tmp/vaa3d-swc-color-gui
/tmp/vaa3d-swc-color-gui.o: ../../tests/swc_color_gui_test.cpp ../3drenderer/v3dr_surfaceDialog.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-swc-color-gui: /tmp/vaa3d-swc-color-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

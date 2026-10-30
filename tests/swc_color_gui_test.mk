.PHONY: swc-color-gui-test
swc-color-gui-test: /tmp/vaa3d-swc-color-gui
/tmp/vaa3d-swc-color-gui.o: ../../tests/swc_color_gui_test.cpp main.cpp ../3drenderer/v3dr_surfaceDialog.h ../3drenderer/renderer_gl1.h ../3drenderer/v3dr_glwidget.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-swc-color-gui: /tmp/vaa3d-swc-color-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

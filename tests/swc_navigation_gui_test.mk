.PHONY: swc-navigation-gui-test
swc-navigation-gui-test: /tmp/vaa3d-swc-navigation-gui
/tmp/vaa3d-swc-navigation-gui.o: ../../tests/swc_navigation_gui_test.cpp main.cpp ../3drenderer/renderer_gl1.h ../3drenderer/v3dr_glwidget.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -UNDEBUG -o $@ $<
/tmp/vaa3d-swc-navigation-gui: /tmp/vaa3d-swc-navigation-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

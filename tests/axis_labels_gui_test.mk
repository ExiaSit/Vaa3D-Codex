.PHONY: axis-labels-gui-test
axis-labels-gui-test: /tmp/vaa3d-axis-labels-gui
/tmp/vaa3d-axis-labels-gui.o: ../../tests/axis_labels_gui_test.cpp ../3drenderer/AxisLabelLayout.h ../3drenderer/v3dr_glwidget.h ../basic_c_fun/basic_surf_objs.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-axis-labels-gui: /tmp/vaa3d-axis-labels-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

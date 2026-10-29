.PHONY: voxel-spacing-gui-test
voxel-spacing-gui-test: /tmp/vaa3d-voxel-spacing-gui
/tmp/vaa3d-voxel-spacing-gui.o: ../../tests/voxel_spacing_gui_test.cpp ../3drenderer/v3dr_glwidget.h ../3drenderer/v3dr_mainwindow.h ../3drenderer/renderer.h ../3drenderer/renderer_gl1.h ../3drenderer/renderer_gl2.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-voxel-spacing-gui: /tmp/vaa3d-voxel-spacing-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
